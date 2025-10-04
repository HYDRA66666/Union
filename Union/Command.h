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
#include "GlobalThreadLake.h"
#include "commander_streambuf.h"

namespace HYDRA15::Union::commander
{
    // 接管 std 输入、输出，处理指令、调用处理函数
    // 程序需要在启动初期注册指令和对应的处理函数
    // 处理函数不得有返回值，输入为参数列表，列表的首项一致为命令本身
    // 指令处理函数可以是同步的也可以是异步的，但是有与控制台交互的指令必须是同步的
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
    private:
        static struct visualize
        {
            static_string prompt = "> ";
            static_string onExit = "Press enter to exit...";
            static_string unknownExptDuringExcute = "Unknown exception occured during excuting command > {}";
        }vslz;

        // 配置
    private:
        static struct config
        {

        }cfg;
        std::function<void(const std::string&)> sysprint;
        std::function<std::string()> sysgetline;



        /***************************** 输入输出 *****************************/
    private:
        std::streambuf* pSysOutBuf = nullptr;
        ostreambuf* pCmdOutBuf = nullptr;
        std::ostream* pSysOutStream = nullptr;

        std::streambuf* pSysInBuf = nullptr;
        istreambuf* pCmdInBuf = nullptr;
        std::istream* pSysInStream = nullptr;

    //private:
    //    bool inputting = false;
    //    std::string currentInputLine;
    //    std::mutex inputMutex;
    //    std::condition_variable inputCv;
    public:
        static std::string getline();
        static std::string getline(const std::string& promt);
    //    std::string getline();
    //    std::string getline(std::string promt);

        /***************************** 指令处理 *****************************/
        // 指令处理函数类型
    public:
        using command_handler = std::function<void(const std::list<std::string>& args)>;

        // 线程池相关
    private:
        GlobalThreadLake& globalthreadpool = GlobalThreadLake::get_instance();

        // 指令注册与使用
    private:
        archivist::basic_registry<std::string, std::pair<bool, command_handler>> cmdRegistry;
        mutable std::shared_mutex cmdRegMutex; 
    public:
        void regist(const std::string& cmd, bool async, const command_handler& handler);
        bool unregist(const std::string& cmd);
        bool contains(const std::string& cmd) const;
        void excute(const std::string& cmdline);

        // 指令处理
    private:
        bool working = true;
        virtual void work(background::thread_info& info) override;
        static void handler_shell(const command_handler& handler, const std::list<std::string>& args);

        /***************************** 快捷命令 *****************************/
    public:
        static void regist_command(const std::string& cmd, bool async, const command_handler& handler);
    };
    
}
