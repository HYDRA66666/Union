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
                return;
            }

        // 没有，则入队
        inst.setlineQueue.emplace_back(id, line);
        return;
    }


    ScanCenter::ScanCenter(bool waitForSignal)
        :labourer::background(1)
    {
        if (!waitForSignal)
        {
            working = true;
            labourer::background::start();
        }
    }


    ScanCenter::~ScanCenter()
    {
        working = false;
    }

    ScanCenter& ScanCenter::get_instance(bool waitForSignal)
    {
        static ScanCenter* instance = new ScanCenter{ waitForSignal };
        return *instance;
    }

    void ScanCenter::work(thread_info& info)
    {
        while (true)
        {
            {
                std::unique_lock ul(queueLock);
                if (!getlineQueue.empty())
                    PrintCenter::set_stick_btm(getlineQueue.front().promt);
                else
                    PrintCenter::set_stick_btm(defaultPromt);
            }
            std::string ln = sysgetline();
            {
                std::unique_lock ul(queueLock);
                if (!getlineQueue.empty())
                {
                    getlineQueue.front().prms.set_value(ln);
                    getlineQueue.pop_front();
                    continue;
                }
            }
            if (sysassign)
            {
                std::thread(sysassign, ln).detach();
                continue;
            }
        }
    }

    void ScanCenter::set_getline(std::function<std::string()> g)
    {
        sysgetline = g;
    }

    void ScanCenter::set_defaultPromt(const std::string& promt)
    {
        defaultPromt = promt;
    }

    void ScanCenter::start()
    {
        bool currentState = false;
        if (working.compare_exchange_strong(currentState, true))
            labourer::background::start();
    }

    void ScanCenter::set_assign(std::function<void(const std::string&)> a)
    {
        sysassign = a;
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
