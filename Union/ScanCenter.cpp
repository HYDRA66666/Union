#include "ScanCenter.h"

namespace HYDRA15::Union::secretary
{
    ScanCenter::ScanCenter(bool waitForSignal)
        :labourer::background(1)
    {
        if (!waitForSignal)
        {
            working = true;
            labourer::background::start();
        }
    }

    std::string ScanCenter::getline(std::string promt)
    {
        ScanCenter& inst = ScanCenter::get_instance();
        std::unique_lock ul(inst.inputMutex);
        PrintCenter::get_instance().set_stick_btm(promt);
        PrintCenter::get_instance().sync_flush();
        std::unique_lock lul(inst.inputLineMutex);
        inst.isWaiting = true;
        while (inst.line.empty())
            inst.inputcv.wait(lul);
        inst.isWaiting = false;
        return std::move(inst.line);
    }

    std::string ScanCenter::getline()
    {
        return getline(vslz.promt.data());
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
            std::string ln = sysgetline();
            PrintCenter::get_instance().set_stick_btm(defaultPromt);
            PrintCenter::get_instance().sync_flush();
            if (isWaiting)
            {
                std::unique_lock ul(inputLineMutex);
                line = ln;
                inputcv.notify_all();
            }
            else if (sysassign)
                std::thread(sysassign, ln).detach();
            else
            {
                std::unique_lock ul(inputLineMutex);
                line = ln;
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

}
