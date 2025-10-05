#include "pch.h"
#include "Command.h"

namespace HYDRA15::Union::commander
{
    Command::Command()
        : labourer::background(1)
    {
        // 重定向输出
        {
            std::lock_guard lg(secretary::PrintCenter::get_instance());
            pCmdOutBuf = new ostreambuf();
            pSysOutBuf = std::cout.rdbuf(pCmdOutBuf);
            std::ostream* psos = pSysOutStream = new std::ostream(pSysOutBuf);
            sysprint = [psos](const std::string& str) { *psos << str; };
            secretary::PrintCenter::get_instance().redirect(sysprint);
        }

        // 重定向输入
        {
            pCmdInBuf = new istreambuf([this]() {return getline(); }, this->begin().get_id());
            pSysInBuf = std::cin.rdbuf(pCmdInBuf);
            std::cin.exceptions(std::ios::badbit | std::ios::failbit);
            std::istream* psis = pSysInStream = new std::istream(pSysInBuf);
            sysgetline = [psis]() { std::string res; std::getline(*psis, res); return res; };
        }
        //sysgetline = []() { std::string res; std::getline(std::cin, res); return res; };

        // 启动后台线程
        asyncInput.start();
        start();
    }

    Command::~Command()
    {
        // 停止后台线程
        working = false;
        
        // 结束输入
        asyncInput.working = false;
        asyncInput.notify_all();

        // 恢复输出
        {
            std::lock_guard lg(secretary::PrintCenter::get_instance());
            std::cout.rdbuf(pSysOutBuf);
            delete pSysOutStream;
            delete pCmdOutBuf;
            secretary::PrintCenter::get_instance().redirect([](const std::string& str) {std::cout << str; });
        }

        // 恢复输入
        {
            std::cin.rdbuf(pSysInBuf);
            delete pSysInStream;
            delete pCmdInBuf;
            //sysgetline = []() { std::string res; std::getline(std::cin, res); return res; };
        }


    }

    Command& Command::get_instance()
    {
        static Command instance;
        return instance;
    }

    //std::string Command::getline()
    //{
    //    std::unique_lock lg(inputMutex);
    //    inputting = true;
    //    while(currentInputLine.empty())
    //        inputCv.wait(lg);
    //    inputting = false;
    //    return std::move(currentInputLine);
    //}

    //std::string Command::getline(std::string promt)
    //{
    //    secretary::PrintCenter::get_instance().set_stick_btm(promt);
    //    return getline();
    //}

    void Command::set_strict_destruct(bool sd)
    {
        strictDestruct = sd;
    }

    std::string Command::getline()
    {
        if(std::this_thread::get_id() != Command::get_instance().begin().get_id())
            throw exceptions::commander::CommandAsyncInputNotAllowed();
        return Command::get_instance().asyncInput.get_line_high_priority();
    }

    std::string Command::getline(const std::string& promt)
    {
        if (std::this_thread::get_id() != Command::get_instance().begin().get_id())
            throw exceptions::commander::CommandAsyncInputNotAllowed();
        secretary::PrintCenter::get_instance().set_stick_btm(promt);
        secretary::PrintCenter::get_instance().flush();
        return getline();
    }

    void Command::regist(const std::string& cmd, bool async, const command_handler& handler)
    {
        std::unique_lock ul(cmdRegMutex);
        cmdRegistry.regist(cmd, std::pair{ async,handler });
    }

    bool Command::unregist(const std::string& cmd)
    {
        std::unique_lock ul(cmdRegMutex);
        return cmdRegistry.unregist(cmd);
    }

    bool Command::contains(const std::string& cmd) const
    {
        std::shared_lock sl(cmdRegMutex);
        return cmdRegistry.contains(cmd);
    }

    void Command::excute(const std::string& cmdline)
    {
        std::list<std::string> args = assistant::split_by(cmdline, " ");
        std::string cmd = args.front();
        std::pair<bool, command_handler> cmdHandler;

        if(cmdRegistry.contains(cmd))
        {
            std::shared_lock sl(cmdRegMutex);
            cmdHandler = cmdRegistry.fecth(cmd);
        }
        else if(cmdRegistry.contains(std::string()))
        {
            std::shared_lock sl(cmdRegMutex);
            cmdHandler = cmdRegistry.fecth(std::string());
        }
        else
        {
            lgr.error(exceptions::commander::NoSuchCommand(cmdline).what());
            return;
        }

        if (cmdHandler.first)
            globalthreadpool.submit(std::bind(handler_shell, cmdHandler.second, args));
        else
            handler_shell(cmdHandler.second, args);
    }

    void Command::work(background::thread_info& info)
    {
        while (working)
        {
            secretary::PrintCenter::get_instance().set_stick_btm(vslz.prompt.data());
            secretary::PrintCenter::get_instance().flush();

            std::string cmdline = asyncInput.get_line();
            if(cmdline.empty())
                continue;
            //if(inputting)
            //{
            //    std::lock_guard lg(inputMutex);
            //    currentInputLine = cmdline;
            //    inputCv.notify_all();
            //    continue;
            //}

            excute(cmdline);
        }
    }

    void Command::handler_shell(const command_handler& handler, const std::list<std::string>& args)
    {
        try
        {
            handler(args);
        }
        catch (const std::exception& e)
        {
            Command::get_instance().lgr.error(e.what());
        }
        catch (...)
        {
            Command::get_instance().lgr.error(vslz.unknownExptDuringExcute.data());
        }
    }

    void Command::regist_command(const std::string& cmd, bool async, const command_handler& handler)
    {
        get_instance().regist(cmd, async, handler);
    }
    void Command::regist_default_command(bool async, const command_handler& handler)
    {
        get_instance().regist(std::string(), async, handler);
    }

    void Command::async_input::work(background::thread_info& info)
    {
        Command& cmd = Command::get_instance();
        while (true)
        {
            std::string ln = cmd.sysgetline();
            std::lock_guard lg(inputMutex);
            line = ln;
            notify_all();
        }
    }
    std::string Command::async_input::get_line()
    {
        std::unique_lock ul(inputMutex);
        while(line.empty() && working)
            inputCv.wait(ul);
        return std::move(line);
    }

    std::string Command::async_input::get_line_high_priority()
    {
        std::unique_lock ul(inputMutex);
        while (line.empty() && working)
            highPriorityCv.wait(ul);
        return std::move(line);
    }

    void Command::async_input::notify_all()
    {
        highPriorityCv.notify_all();
        inputCv.notify_all();
    }
}