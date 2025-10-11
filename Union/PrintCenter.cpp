#include "pch.h"
#include "PrintCenter.h"

namespace HYDRA15::Union::secretary
{
    unsigned long long PrintCenter::set(const std::string& str, bool forceDisplay, bool neverExpire)
    {
        PrintCenter& instance = get_instance();
        ID ret = instance.new_bottom(forceDisplay, neverExpire);
        instance.update_bottom(ret, str);
        instance.flush();
        return ret;
    }

    bool PrintCenter::update(unsigned long long id, const std::string& str)
    {
        try
        {
            PrintCenter& instance = get_instance();
            instance.update_bottom(id, str);
            instance.flush();
        }
        catch (referee::iExceptionBase&)
        {
            return false;
        }
        return true;
    }

    bool PrintCenter::remove(unsigned long long id)
    {
        try
        {
            get_instance().remove_bottom(id);
        }
        catch (referee::iExceptionBase&)
        {
            return false;
        }
        return true;
    }

    void PrintCenter::set_stick_btm(const std::string& str)
    {
        get_instance().stick_btm(str);
        get_instance().sync_flush();
    }

    size_t PrintCenter::fprint(const std::string& str)
    {
        PrintCenter& instance = PrintCenter::get_instance();
        size_t ret = instance.file(str);
        instance.flush();
        return ret;
    }

    PrintCenter& PrintCenter::operator<<(const std::string& content)
    {
        if (print)
            rolling(content);
        if (printFile)
            file(assistant::strip_color(content));
        flush();
        return *this;
    }

    PrintCenter::PrintCenter()
        :labourer::background(1)
    {
        start();
    }

    PrintCenter& PrintCenter::get_instance()
    {
        static PrintCenter instance;
        return instance;
    }

    PrintCenter::~PrintCenter()
    {
        working = false;
        sleepcv.notify_all();
        wait_for_end();
    }

    std::string PrintCenter::clear_bottom_msg()
    {
        using namespace HYDRA15::Union::assistant;

        std::string str = "\r\033[2K";
        if (lastBtmLines > 1)
            str += std::string("\033[1A\033[2K") * (lastBtmLines - 1);
        lastBtmLines = 0;
        return str;
    }

    std::string PrintCenter::print_rolling_msg()
    {
        {   // 交换缓冲区
            std::lock_guard lg(rollMsgLock);
            rollmsg_list* temp = pRollMsgLstFront;
            pRollMsgLstFront = pRollMsgLstBack;
            pRollMsgLstBack = temp;
        }

        std::string str;
        for (auto& msg : *pRollMsgLstBack)
            str.append(assistant::strip(msg, is_valid_with_ansi) + "\n");
        pRollMsgLstBack->clear();

        return str;
    }

    std::string PrintCenter::print_bottom_msg()
    {
        size_t more = 0;
        std::list<ID> expires;
        std::string str;
        bool first = true;

        std::lock_guard lk(btmMsgTabLock);
        time_point now = time_point::clock::now();
        for(auto& [id,i]: btmMsgTab)
        {
            if (i.msg.empty())
                continue;
            if(!i.neverExpire && now - i.lastUpdate > cfg.btmExpireTimeout)
            {
                expires.push_back(id);
                continue;
            }
            if (!i.forceDisplay && (now - i.lastUpdate > cfg.btmDispTimeout || lastBtmLines >= cfg.btmMaxLines))
            {
                more++;
                continue;
            }
            if(!first)
                str.append("\n");
            else
                first = false;
            str.append(assistant::strip(i.msg, is_valid_with_ansi));
            lastBtmLines++;
        }
        if (more > 0)
        {
            if(lastBtmLines > 0)
                str.append("\n");
            str.append(std::format(cfg.btmMoreFormat.data(), more));
            lastBtmLines++;
        }

        for(const auto& id : expires)
            btmMsgTab.unregist(id);

        if(!stickBtmMsg.empty())
        {
            if (!first)
                str.append("\n");
            else
                first = false;
            str.append(stickBtmMsg);
            lastBtmLines++;
        }

        return str;
    }

    std::string PrintCenter::print_file_msg()
    {
        {
            std::lock_guard lg(fileMsgLock);
            filemsg_list* temp = pFMsgLstBack;
            pFMsgLstBack = pFMsgLstFront;
            pFMsgLstFront = temp;
        }

        std::string str;

        for (const auto& msg : *pFMsgLstBack)
            str.append(assistant::strip(msg) + "\n");
        pFMsgLstBack->clear();

        return str;
    }

    void PrintCenter::work(background::thread_info&)
    {
        while (
            working || // 工作中
            ((!pRollMsgLstBack->empty() || !pRollMsgLstFront->empty()) && print) || // 滚动消息不为空且可打印
            (btmMsgTab.size() > 0 && print) || // 底部消息不为空且可打印
            ((!pFMsgLstBack->empty() || !pFMsgLstFront->empty()) && printFile) // 文件消息不为空且可打印
            )
        {
            std::unique_lock lg(systemLock);

            // 无工作，等待
            while (
                working && // 工作中
                ((pRollMsgLstFront->empty() && pRollMsgLstBack->empty()) || !print) && // 滚动消息为空
                ((pFMsgLstBack->empty() && pFMsgLstFront->empty()) || !printFile) && // 文件消息为空
                (time_point::clock::now() - lastRefresh < cfg.refreshInterval) && // 未到刷新时间
                !forceRefresh   // 未被强制刷新
                )
                sleepcv.wait_for(lg, cfg.refreshInterval);

            // 清除底部消息
            if (print)
                print(clear_bottom_msg());

            // 输出滚动消息
            if (!pRollMsgLstFront->empty())
                if (print)
                    print(print_rolling_msg());

            // 输出底部消息
            if (btmMsgTab.size() > 0 || !stickBtmMsg.empty())
                if (print)
                    print(print_bottom_msg());

            // 输出文件消息
            if (printFile)
                if (!pFMsgLstFront->empty())
                    printFile(print_file_msg());

            // 计时
            lastRefresh = time_point::clock::now();

            // 通知等待
            forceRefresh = false;
            sleepcv.notify_all();
        }
    }

    void PrintCenter::flush()
    {
        std::unique_lock ul(systemLock);
        forceRefresh = true;
        sleepcv.notify_all();
    }

    void PrintCenter::sync_flush()
    {
        std::unique_lock ul(systemLock);
        forceRefresh = true;
        sleepcv.notify_all();
        while (forceRefresh)
            sleepcv.wait(ul);
    }

    void PrintCenter::lock()
    {
        systemLock.lock();
    }

    void PrintCenter::unlock()
    {
        systemLock.unlock();
    }

    void PrintCenter::redirect(std::function<void(const std::string&)> printFunc)
    {
        print = printFunc;
    }

    void PrintCenter::fredirect(std::function<void(const std::string&)> fprintFunc)
    {
        printFile = fprintFunc;
    }

    size_t PrintCenter::rolling(const std::string& content)
    {
        std::lock_guard lg(rollMsgLock);
        pRollMsgLstFront->push_back(content);
        return rollMsgCount++;
    }

    void PrintCenter::stick_btm(const std::string& str)
    {
        std::lock_guard lk(btmMsgTabLock);
        stickBtmMsg = str;
    }

    PrintCenter::ID PrintCenter::new_bottom(bool forceDisplay, bool neverExpire)
    {
        std::lock_guard lk(btmMsgTabLock);
        try
        {
            return btmMsgTab.regist(btmmsg_ctrlblock{time_point::clock::now(), forceDisplay, neverExpire, std::string() });
        }
        catch(exceptions::archivist& e)
        {
            if (e.exptCode == exceptions::archivist::iException_codes::RegistryTabletFull)
                throw exceptions::secretary::PrintCenterBtmMsgFull();
            else throw e;
        }
    }

    void PrintCenter::update_bottom(ID id, const std::string& content)
    {
        btmmsg_ctrlblock* pMsgCtrl;
        std::lock_guard lk(btmMsgTabLock);

        pMsgCtrl = &btmMsgTab.fetch(id);
        pMsgCtrl->msg = content;
        pMsgCtrl->lastUpdate = time_point::clock::now();
    }

    bool PrintCenter::check_bottom(ID id)
    {
        std::lock_guard lk(btmMsgTabLock);
        return btmMsgTab.contains(id);
    }

    void PrintCenter::remove_bottom(ID id)
    {
        std::lock_guard lk(btmMsgTabLock);
        btmMsgTab.unregist(id);
    }

    size_t PrintCenter::file(const std::string& content)
    {
        std::lock_guard lg(fileMsgLock);

        pFMsgLstFront->push_back(content);
        return fileMsgCount++;
    }
}
