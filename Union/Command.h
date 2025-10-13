#pragma once
#include "pch.h"
#include "framework.h"

#include "PrintCenter.h"
#include "ThreadLake.h"
#include "registry.h"
#include "background.h"
#include "utility.h"
#include "commander_exception.h"
#include "logger.h"
#include "ScanCenter.h"

namespace HYDRA15::Union::commander
{
    // 程序需要在启动初期注册指令和对应的处理函数，指令为空的处理函数为默认处理函数
    // 处理函数不得有返回值，输入为参数列表，列表的首项一致为命令本身
    class Command
    {
        using command_handler = std::function<void(const std::list<std::string>& args)>;
        /***************************** 快捷命令 *****************************/
    public:
        static void regist_command(const std::string& cmd, const command_handler& handler);
        static void regist_default_command(const command_handler& handler);
        // 启动后台线程执行 默认的执行方式
        static void excute(const std::string& cmdline);
        static void excute(const std::list<std::string>& cmdline);
        // 在调用线程执行
        static void excute_sync(const std::string& cmdline);
        static void excute_sync(const std::list<std::string>& cmdline);

        /***************************** 公  用 *****************************/
        // 单例
    private:
        Command();
        Command(const Command&) = delete;
        Command(Command&&) = delete;

    public:
        ~Command();
        static Command& get_instance();

        // 配置
    private:
        static struct visualize
        {
            static_string prompt = "> ";
            static_string onExit = "Press enter to exit...";
            static_string threadpoolNotDefined = "No thread pool specified, which may cause performance impact.";
            static_string unknownExptDuringExcute = "Unknown exception occured during excuting command > {}";
        }vslz;

        // 日志
    private:
        secretary::logger lgr{ "Commander" };

        // 系统锁
    private:
        std::shared_mutex syslock;

        /***************************** 指令处理 *****************************/
        // 线程池相关
    private:
        std::shared_ptr<labourer::ThreadLake> pthreadpool = nullptr;
    public:
        void threadpool(std::shared_ptr<labourer::ThreadLake> p);

        // 指令注册与使用
    private:
        archivist::basic_registry<std::string, command_handler> cmdRegistry;
        mutable std::shared_mutex cmdRegMutex;

    public:
        void regist(const std::string& cmd, const command_handler& handler);
        bool unregist(const std::string& cmd);
        bool contains(const std::string& cmd) const;
        command_handler fetch(const std::string& cmd);


        // 指令处理
    private:
        static void warp(const command_handler& handler, const std::list<std::string>& args);


    };
    
}
