#pragma once
#include "pch.h"

#define ENTRUST_APPLICATION

// 将应用程序的基础逻辑交由框架处理
// 用户需实现指定的初始化函数和清理函数，以及指令对应的业务逻辑
// 在 main 源文件中 define ENTRUST_APPLICATION 宏并包含此头文件即可启用
// deposit 和 entrust 特性只可二选其一
#ifdef ENTRUST_APPLICATION

#include "Command.h"
#include "commander_exception.h"
#include "logger.h"

int init();
int clean();

enum class state :int
{
    undefined,
    startup,
    working,
    shuttingdown,
}state;


class init_guard
{
    int ret = 0;
    std::string args;
public:
    init_guard(int argc, char** argv)
    {
        state = state::startup;
        for (int i = 0; i < argc; i++)
            args.append(argv[i] + std::string(" "));
        ret = init();
    }
    ~init_guard() { state = state::shuttingdown; clean(); }
    int initcode() const { return ret; }
    const std::string& arguments() { return args; }
};

int main(int argc, char** argv)
{
    HYDRA15::Union::commander::Command& cmd = HYDRA15::Union::commander::Command::get_instance();
    HYDRA15::Union::secretary::logger logger("Entrust main");

    try
    {
        init_guard ig(argc, argv);
        if (ig.initcode() != 0)
            throw HYDRA15::Union::exceptions::commander::EntrustNotInitFaild(ig.initcode());

        state = state::working;
        cmd.excute(ig.arguments());

        while (state == state::working)
        {

        }

        return 0;
    }
    catch (const std::exception& e)
    {
        logger.fatal(e.what());
        return -1;
    }
    catch (...)
    {
        logger.fatal("Unknown fatal exception caught in Entrust main");
        return -1;
    }
}


#endif // ENTRUST_APPLICATION
