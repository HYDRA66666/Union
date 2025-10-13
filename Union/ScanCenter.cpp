#include "ScanCenter.h"

namespace HYDRA15::Union::secretary
{
    std::string ScanCenter::getline(std::string promt, unsigned long long id)
    {
        return getline_async(promt, id).get();
    }

    std::future<std::string> ScanCenter::getline_async(std::string promt, unsigned long long id)
    {
        ScanCenter& inst = ScanCenter::get_instance();
        std::unique_lock ul(inst.queueLock);

        // 检查输入列表中是否有想要的
        for(auto it = inst.setlineQueue.begin();it!=inst.setlineQueue.end();it++)
            if (it->id == id)
            {
                std::promise<std::string> prms;
                prms.set_value(it->line);
                inst.setlineQueue.erase(it);
                return std::move(prms.get_future());
            }

        // 没有，则入队
        inst.getlineQueue.emplace_back(id, promt);
        std::future<std::string> fut = inst.getlineQueue.back().prms.get_future();
        PrintCenter::set_stick_btm(inst.getlineQueue.front().promt);
        return fut;
    }

    void ScanCenter::setline(std::string line, unsigned long long id)
    {
        ScanCenter& inst = ScanCenter::get_instance();
        std::unique_lock ul(inst.queueLock);

        // 检查等待列表中有没有目标
        for(auto it = inst.getlineQueue.begin();it!=inst.getlineQueue.end();it++)
            if (it->id == id)
            {
                it->prms.set_value(line);
                inst.getlineQueue.erase(it);
                inst.syscv.notify_all();
                return;
            }

        // 没有，则入队
        inst.setlineQueue.emplace_back(id, line);
        return;
    }


    ScanCenter::ScanCenter()
        :labourer::background(1)
    {
        // 重定向cin
        pSCIstreamBuf = std::make_shared<istreambuf>([this]() {return getline("cin > "); });
        std::streambuf* pSysIstreamBuf = std::cin.rdbuf(pSCIstreamBuf.get());
        pSysInStream = std::make_shared<std::istream>(pSysIstreamBuf);
        sysgetline = [this]() {std::string str; std::getline(*pSysInStream, str); return str; };

        start();
    }


    ScanCenter::~ScanCenter()
    {
        working = false;

        // 等待输入任务全部完成
        std::unique_lock ul(queueLock);
        while (!getlineQueue.empty())
            syscv.wait(ul);

        // 回复 cin
        sysgetline = nullptr;
        std::cin.rdbuf(pSysInStream->rdbuf());
        pSysInStream = nullptr;
        pSCIstreamBuf = nullptr;
    }

    ScanCenter& ScanCenter::get_instance()
    {
        static ScanCenter* instance = new ScanCenter{};
        return *instance;
    }

    void ScanCenter::work(thread_info& info)
    {
        try
        {
            while (working)
            {
                {
                    std::unique_lock ul(queueLock);
                    if (!getlineQueue.empty())
                        PrintCenter::set_stick_btm(getlineQueue.front().promt);
                    else
                        PrintCenter::set_stick_btm(vslz.promt.data());
                }
                std::string ln = sysgetline();
                {
                    std::unique_lock ul(queueLock);
                    if (!getlineQueue.empty())
                    {
                        getlineQueue.front().prms.set_value(ln);
                        getlineQueue.pop_front();
                        syscv.notify_all();
                        continue;
                    }
                }
                {
                    std::unique_lock ul(sysLock);
                    if (assign)
                    {
                        std::thread(assign, ln).detach();
                        continue;
                    }
                }
                {
                    std::unique_lock ul(queueLock);
                    setlineQueue.push_back({ 0,ln });
                }
            }
        }
        catch (...) { return; }
    }

    void ScanCenter::set_assign(std::function<void(const std::string&)> a)
    {
        std::unique_lock ul(sysLock);
        assign = a;
    }

    bool ScanCenter::getline_request::operator==(unsigned long long i)
    {
        return id == i;
    }

    bool ScanCenter::getline_request::operator==(const getline_request& oth)
    {
        return id == oth.id;
    }

    bool ScanCenter::putline_request::operator==(unsigned long long i)
    {
        return id == i;
    }

    bool ScanCenter::putline_request::operator==(const putline_request& oth)
    {
        return id = oth.id;
    }

}
