#pragma once

// 托管 main 函数仅用于在发生未捕获异常时输出信息
// 在 main 源文件中 define DEPOSIT_MAIN_FUNC 宏并包含此头文件即可启用
// deposit 和 entrust 特性只可二选其一
#ifdef DEPOSIT_MAIN_FUNC

#include "iExceptionBase.h"
#include "log.h"

int deposited_application_main(int argc, char** argv);

int main(int argc, char** argv)
{
    try
    {
        return deposited_application_main(argc, argv);
    }
    catch (const HYDRA15::Union::referee::iExceptionBase& e)
    {
        HYDRA15::Union::secretary::log::fatal("main", e.what());
        return -1;
    }
    catch (const std::exception&)
    {
        HYDRA15::Union::secretary::log::fatal("main", e.what());
        return -1;
    }
}

#define main deposited_application_main

#endif // DEPOST_MAIN_FUNC

