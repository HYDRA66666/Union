#include "ScanCenter.h"

namespace HYDRA15::Union::secretary
{
    std::string ScanCenter::getline(std::string id, std::string promt)
    {
        return async_getline(id, promt).get();
    }

    std::future<std::string> ScanCenter::async_getline(std::string id, std::string promt)
    {
        ScanCenter& inst = ScanCenter::get_instance();
        std::unique_lock ul(inst.inputQueueMutex);
        input_ctrlblk ic{ id,promt };
        std::future<std::string> fut = ic.prms.get_future();
        inst.inputQueue.emplace_back(std::move(ic));
        return std::move(fut);
    }


    ScanCenter& ScanCenter::get_instance()
    {
        static ScanCenter* instance = new ScanCenter{};
        return *instance;
    }

    void ScanCenter::work(thread_info& info)
    {
        while (working)
        {
            input_ctrlblk ic;
            {
                std::unique_lock ul(inputQueueMutex);
                if (inputQueue.empty() && !sysassign)
                    syscv.wait(ul);
                if (!inputQueue.empty())
                {
                    ic = std::move(inputQueue.front());
                    inputQueue.pop_front();
                }
            }
            while(true)
            {
                try
                {
                    if (ic)
                    {
                        pc.set_stick_btm(std::format(vslz.promtFormat.data(), ic.id, ic.prms));
                        ic.prms.set_value(sysgetline());
                    }
                    else
                    {
                        if (sysassign)
                            sysassign(sysgetline());
                    }
                    break;
                }
                catch (...) { continue; }
            }
        }
    }

    void ScanCenter::set_getline(std::function<std::string()> g)
    {
        sysgetline = g;
    }

    void ScanCenter::set_assign(std::function<void(std::string)> a)
    {
        sysassign = a;
    }

}
