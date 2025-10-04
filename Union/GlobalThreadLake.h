#pragma once
#include "framework.h"
#include "pch.h"

#include "ThreadLake.h"

namespace HYDRA15::Union::commander
{
    static unsigned int default_thread_count = std::thread::hardware_concurrency();

    class GlobalThreadLake : public labourer::ThreadLake
    {
        // 单例模式
    private:
        GlobalThreadLake();
        GlobalThreadLake(const GlobalThreadLake&) = delete;
        GlobalThreadLake(GlobalThreadLake&&) = delete;

    public:
        ~GlobalThreadLake() = default;
        static GlobalThreadLake& get_instance();
    };
}