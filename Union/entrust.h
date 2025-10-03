#pragma once
#include "pch.h"

#define ENTRUST_APPLICATION

// 将应用程序的基础逻辑交由框架处理
// 用户需实现指定的初始化函数和清理函数，以及指令对应的业务逻辑
// 在 main 源文件中 define ENTRUST_APPLICATION 宏并包含此头文件即可启用
// deposit 和 entrust 特性只可二选其一
#ifdef ENTRUST_APPLICATION

int init(std::list<std::string> args);
int clean();


class init_guard
{
    int ret = 0;
    std::list<std::string> args;
public:
    init_guard(int argc, char** argv)
    {
        for (int i = 0; i < argc; i++)
            args.push_back(argv[i]);
        ret = init(args);
    }
    ~init_guard() { clean(); }
    int code() const { return ret; }
    std::list<std::string>& arguments() { return args; }
};

int main(int argc, char** argv)
{

}


#endif // ENTRUST_APPLICATION
