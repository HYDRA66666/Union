#pragma once
#include "framework.h"
#include "pch.h"

#include "background.h"
#include "datetime.h"
#include "string_utilities.h"
#include "secretary_streambuf.h"

namespace HYDRA15::Union::secretary
{
    // 统一输出接口
    // 提供滚动消息、底部消息和写入文件三种输出方式
    // 提交消息之后，调用 notify() 方法通知后台线程处理，这在连续提交消息时可以解约开销
    class PrintCenter final :protected labourer::background
    {
        /***************************** 快速接口 *****************************/
    public:
        template<typename ...Args>
        static size_t println(Args ... args)
        {
            PrintCenter& instance = get_instance();
            std::stringstream ss;
            (ss << ... << args);
            size_t ret = instance.rolling(ss.str());
            instance.flush();
            return ret;
        }

        template<typename ... Args>
        static size_t printf(const std::string& fstr, Args...args) { return println(std::vformat(fstr, std::make_format_args(args...))); }

        static unsigned long long set(const std::string& str, bool forceDisplay = false, bool neverExpire = false)
        {
            PrintCenter& instance = get_instance();
            ID ret = instance.new_bottom(forceDisplay, neverExpire);
            instance.update_bottom(ret, str);
            instance.flush();
            return ret;
        }

        static void update(unsigned long long id, const std::string& str)
        {
            PrintCenter& instance = get_instance();
            instance.update_bottom(id, str);
            instance.flush();
        }

        static void remove(unsigned long long id) { get_instance().remove_bottom(id); }

        static void set_stick_btm(const std::string& str)
        {
            get_instance().stick_btm(str);
            get_instance().sync_flush();
        }

        static size_t fprint(const std::string& str)
        {
            PrintCenter& instance = PrintCenter::get_instance();
            size_t ret = instance.file(str);
            instance.flush();
            return ret;
        }

        PrintCenter& operator<<(const std::string& content)    // 快速输出，滚动消息+文件+刷新
        {
            if (print)
                rolling(content);
            if (printFile)
                file(assistant::strip_color(content));
            flush();
            return *this;
        }

        /***************************** 公有单例 *****************************/
    protected:
        // 禁止外部构造
        PrintCenter()
            :labourer::background(1)
        {
            // 重定向 cout
            pPCOutBuf = std::make_shared<ostreambuf>([this](const std::string& str) {this->println( str); });
            std::streambuf* pSysOstreamBuf = std::cout.rdbuf(pPCOutBuf.get());
            pSysOutStream = std::make_shared<std::ostream>(pSysOstreamBuf);
            print = [this](const std::string& str) { if (enableAnsi) *pSysOutStream << str; else *pSysOutStream << assistant::strip_ansi_secquence(str); };

            // 启动清屏
            print("\0x1B[2J\0x1B[H");

            start();
        }

        PrintCenter(const PrintCenter&) = delete;

        // 获取接口
    public:
        static PrintCenter& get_instance() { static PrintCenter instance; return instance; }

    public:
        ~PrintCenter()
        {
            {
                std::unique_lock ul{ systemLock };
                // 恢复 cout
                std::cout.rdbuf(pSysOutStream->rdbuf());
                print = [](const std::string& str) {if (enableAnsi)std::cout << str; else std::cout << assistant::strip_ansi_secquence(str); };
            }
            pSysOutStream = nullptr;
            pPCOutBuf = nullptr;

            working.store(false, std::memory_order_release);
            sleepcv.notify_all();
            wait_for_end();

            // 结束清除末尾行
            print(clear_bottom_msg());
        }

        /***************************** 公 用 *****************************/
        // 类型
    private:
        using time_point = std::chrono::steady_clock::time_point;
        using milliseconds = std::chrono::milliseconds;

        // 全局配置
    private:
        static struct Config
        {
            static constexpr milliseconds refreshInterval = milliseconds(30000); // 最短刷新间隔

            static_uint btmMaxLines = 3;
            static constexpr milliseconds btmDispTimeout = milliseconds(1000);
            static constexpr milliseconds btmExpireTimeout = milliseconds(30000);
            
        }cfg;
        std::function<bool(char)> is_valid_with_ansi = [](char c) {return (c > 0x20 && c < 0x7F) || c == 0x1B; };


        // 辅助函数
    private:
        std::string clear_bottom_msg()    // 清除底部消息
        {
            using namespace HYDRA15::Union::assistant;

            std::string str = "\r\033[2K";
            if (lastBtmLines > 1)
                str += std::string("\033[1A\033[2K") * (lastBtmLines - 1);
            lastBtmLines = 0;
            return str;
        }

        std::string print_rolling_msg() // 输出滚动消息
        {
            {   // 交换缓冲区
                std::lock_guard lg(rollMsgLock);
                rollmsg_list* temp = pRollMsgLstFront;
                pRollMsgLstFront = pRollMsgLstBack;
                pRollMsgLstBack = temp;
            }

            std::string str;
            for (auto& msg : *pRollMsgLstBack)
                str.append(msg + "\n");
            //if (enableAnsiColor)
            //    str.append(assistant::strip(msg, is_valid_with_ansi) + "\n");
            //else
            //    str.append(assistant::strip_color(assistant::strip(msg, is_valid_with_ansi) + "\n"));
            pRollMsgLstBack->clear();

            return str;
        }

        std::string print_bottom_msg()  // 输出底部消息
        {
            size_t more = 0;
            std::list<ID> expires;
            std::string str;
            bool first = true;

            std::lock_guard lk(btmMsgTabLock);
            time_point now = time_point::clock::now();
            for (auto& [id, i] : btmMsgTab)
            {
                if (i.msg.empty())
                    continue;
                if (!i.neverExpire && now - i.lastUpdate > cfg.btmExpireTimeout)
                {
                    expires.push_back(id);
                    continue;
                }
                if (!i.forceDisplay && (now - i.lastUpdate > cfg.btmDispTimeout || lastBtmLines >= cfg.btmMaxLines))
                {
                    more++;
                    continue;
                }
                if (!first)
                    str.append("\n");
                else
                    first = false;
                str.append(assistant::strip(i.msg, is_valid_with_ansi));
                lastBtmLines++;
            }
            if (more > 0)
            {
                if (lastBtmLines > 0)
                    str.append("\n");
                str.append(std::format(" ... and {0} more", more));
                lastBtmLines++;
            }

            for (const auto& id : expires)
                btmMsgTab.erase(id);

            if (!stickBtmMsg.empty())
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

        std::string print_file_msg()    // 输出文件消息
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


        // 重定向时修改此变量
        std::shared_ptr<ostreambuf> pPCOutBuf;
        std::shared_ptr<std::ostream> pSysOutStream;
        std::function<void(const std::string&)> print;
        std::function<void(const std::string&)> printFile;

        // 是否启用 ansi 控制串
        // 由于代码中大量使用 ansi 控制串进行光标控制和清屏等操作，禁用可能会导致输出不符合预期，建议仅在不支持 ansi 控制串的终端中禁用
    public:
        static inline bool enableAnsi = true;

        // 工作
    private:
        std::condition_variable sleepcv;
        std::mutex systemLock;
        std::atomic<bool> working = true;
        std::atomic<bool> forceRefresh = false;
        time_point lastRefresh = time_point::clock::now();
        virtual void work() noexcept override
        {
            while (
                working.load(std::memory_order_acquire) || // 工作中
                ((!pRollMsgLstBack->empty() || !pRollMsgLstFront->empty()) && print) || // 滚动消息不为空且可打印
                (btmMsgTab.size() > 0 && print) || // 底部消息不为空且可打印
                ((!pFMsgLstBack->empty() || !pFMsgLstFront->empty()) && printFile) // 文件消息不为空且可打印
                )
            {
                std::unique_lock lg(systemLock);

                // 无工作，等待
                while (
                    working.load(std::memory_order_acquire) && // 工作中
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

        // 高级接口
    public:
        void flush()  // 刷新
        {
            std::unique_lock ul(systemLock);
            forceRefresh = true;
            sleepcv.notify_all();
        }

        void sync_flush()  // 同步刷新，后台线程刷新完成后才会返回
        {
            std::unique_lock ul(systemLock);
            forceRefresh = true;
            sleepcv.notify_all();
            while (forceRefresh)
                sleepcv.wait(ul);
        }

        void lock() { systemLock.lock(); }   // 锁定，防止刷新

        void unlock() { systemLock.unlock(); } // 解锁，允许刷新

        void fredirect(std::function<void(const std::string&)> fprintFunc) { printFile = fprintFunc; }


        /***************************** 滚动消息相关 *****************************/
        // 类型定义
    private:
        using rollmsg_list = std::list<std::string>;

        // 数据
    private:
        rollmsg_list* pRollMsgLstFront = new rollmsg_list();
        rollmsg_list* pRollMsgLstBack = new rollmsg_list();
        std::mutex rollMsgLock;
        size_t rollMsgCount = 0;

        // 接口
    public:
        size_t rolling(const std::string& content)
        {
            std::lock_guard lg(rollMsgLock);
            pRollMsgLstFront->push_back(content);
            return rollMsgCount++;
        }


        /***************************** 底部消息相关 *****************************/
       // 类型定义
    private:
        struct btmmsg_ctrlblock
        {
            time_point lastUpdate = time_point::clock::now();
            bool forceDisplay = false;
            bool neverExpire = false;
            std::string msg;
        };

    public:
        using ID = unsigned long long;
    private:
        using btmmsg_tab = std::unordered_map<ID, btmmsg_ctrlblock>;
        

        // 数据
    private:
        std::string stickBtmMsg;
        btmmsg_tab btmMsgTab;
        ID btmMsgNextID = 0;
        std::mutex btmMsgTabLock;
        size_t lastBtmLines = 0;

        // 工具函数
    private:
        ID find_next_ID()
        {
            if (!btmMsgTab.contains(btmMsgNextID))
                return btmMsgNextID;
            while (btmMsgTab.contains(btmMsgNextID) && btmMsgNextID != std::numeric_limits<ID>::max())
                btmMsgNextID++;
            if (btmMsgTab.contains(btmMsgNextID) && btmMsgNextID == std::numeric_limits<ID>::max()) // 若达到最大值，则重新扫描整整表，查找是否有空缺位置
                btmMsgNextID = 0;
            while (btmMsgTab.contains(btmMsgNextID) && btmMsgNextID != std::numeric_limits<ID>::max())
                btmMsgNextID++;
            return btmMsgNextID;
        }

        // 接口
    public:
        ID new_bottom(bool forceDisplay = false, bool neverExpire = false)
        {
            std::lock_guard lk(btmMsgTabLock);
            ID id = find_next_ID();
            btmMsgTab.emplace(std::pair<ID, btmmsg_ctrlblock>{
                id,
                    btmmsg_ctrlblock{
                        time_point::clock::now(),
                        forceDisplay,
                        neverExpire,
                        std::string()
                }
            });
            btmMsgNextID++;
            return id;

        }

        void update_bottom(ID id, const std::string& content)
        {
            btmmsg_ctrlblock* pMsgCtrl;
            std::lock_guard lk(btmMsgTabLock);

            pMsgCtrl = &btmMsgTab.at(id);
            pMsgCtrl->msg = content;
            pMsgCtrl->lastUpdate = time_point::clock::now();
        }

        bool check_bottom(ID id) { std::lock_guard lk(btmMsgTabLock); return btmMsgTab.contains(id); }

        void remove_bottom(ID id) { std::lock_guard lk(btmMsgTabLock); btmMsgTab.erase(id); }

        void stick_btm(const std::string& str = std::string()) { std::lock_guard lk(btmMsgTabLock); stickBtmMsg = str; }



        /***************************** 写入文件相关 *****************************/
        // 类型定义
    private:
        using filemsg_list = std::list<std::string>;

        // 数据
    private:
        filemsg_list* pFMsgLstFront = new filemsg_list();
        filemsg_list* pFMsgLstBack = new filemsg_list();
        std::mutex fileMsgLock;
        size_t fileMsgCount = 0;

        // 接口
    public:
        size_t file(const std::string& content)
        {
            std::lock_guard lg(fileMsgLock);

            pFMsgLstFront->push_back(content);
            return fileMsgCount++;
        }
    };
}
