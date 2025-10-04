#include "pch.h"
#include "GlobalThreadLake.h"

namespace HYDRA15::Union::commander
{
    GlobalThreadLake::GlobalThreadLake()
        : labourer::ThreadLake(default_thread_count)
    {
    }

    GlobalThreadLake& GlobalThreadLake::get_instance()
    {
        static GlobalThreadLake instance;
        return instance;
    }
}