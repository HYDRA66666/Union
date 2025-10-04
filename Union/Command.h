#pragma once
#include "pch.h"
#include "framework.h"

#include "commander_ostreambuf.h"
#include "PrintCenter.h"
#include "ThreadLake.h"
#include "registry.h"
#include "background.h"
#include "utility.h"
#include "commander_exception.h"
#include "logger.h"

namespace HYDRA15::Union::commander
{
    // 接管 std 输入、输出，处理指令、调用处理函数
    // 程序需要在启动初期注册指令和对应的处理函数
    // 处理函数不得有返回值，输入为参数列表，列表的首项意志为命令本身
    class Command :protected labourer::background
    {
        /***************************** 公  用 *****************************/
        // 单例
    private:
        Command();
        Command(const Command&) = delete;
        Command(Command&&) = delete;
        secretary::logger lgr{ "Commander" };

    public:
        ~Command();
        static Command& get_instance();

        // 常量字符串
        static struct visualize
        {
            static_string prompt = "> ";
            static_string onExit = "Press any key to exit...";
            static_string unknownExptDuringExcute = "Unknown exception occured during excuting command > {}";
        }vslz;


        /***************************** 接管输出 *****************************/
    private:
        std::streambuf* pSysOutBuf = nullptr;
        ostreambuf* pCmdOutBuf = nullptr;
        std::ostream* pSysOutStream = nullptr;

        /***************************** 指令处理 *****************************/
        // 指令处理函数类型
    public:
        using command_handler = std::function<void(const std::list<std::string>& args)>;

        // 线程池相关
    private:
        labourer::ThreadLake threadpool{ UNION_DEFAULT_THREAD_COUNT };

        // 指令注册与使用
    private:
        archivist::basic_registry<std::string, std::pair<bool, command_handler>> cmdRegistry;
    public:
        void regist(const std::string& cmd, bool async, const command_handler& handler);
        bool unregist(const std::string& cmd);
        bool contains(const std::string& cmd) const;
        void excute(const std::string& cmdline);

        // 指令处理
    private:
        bool working = true;
        virtual void work(background::thread_info& info) override;
        char inBuf[1024] = { 0 };
        void clear_inbuf();

        /***************************** 快捷命令 *****************************/
    public:
        static void regist_command(const std::string& cmd, bool async, const command_handler& handler);
    };
    
}
