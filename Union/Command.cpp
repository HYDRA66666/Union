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
        cmdQueCv.notify_all();
        wait_for_end();
        
        // 结束输入
        asyncInput.stop();
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


    std::string Command::getline()
    {
        if(std::this_thread::get_id() != Command::get_instance().begin().get_id())
            throw exceptions::commander::CommandAsyncInputNotAllowed();
        return Command::get_instance().asyncInput.get_line();
    }

    std::string Command::getline(const std::string& promt)
    {
        if (std::this_thread::get_id() != Command::get_instance().begin().get_id())
            throw exceptions::commander::CommandAsyncInputNotAllowed();
        secretary::PrintCenter::get_instance().set_stick_btm(promt);
        secretary::PrintCenter::get_instance().flush();
        return getline();
    }

    void Command::threadpool(std::shared_ptr<labourer::ThreadLake> p)
    {
        std::unique_lock ul(syslock);
        pthreadpool = p;
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

    void Command::work(background::thread_info& info)
    {
        while (working || ![this]() {std::unique_lock ul(cmdQueMutex); return cmdQueue.empty(); }())
        {
            std::list<std::string> args;
            {
                std::unique_lock ul(cmdQueMutex);
                while (working && cmdQueue.empty())
                    cmdQueCv.wait(ul);
                if (cmdQueue.empty())
                    continue;
                args = cmdQueue.front();
                cmdQueue.pop();
            }
            if(args.empty())
                continue;

            std::string cmd = args.front();
            std::pair<bool, command_handler> cmdHandler;

            std::unique_lock ul(syslock);

            {
                std::shared_lock sl(cmdRegMutex);
                if (cmdRegistry.contains(cmd))
                    cmdHandler = cmdRegistry.fecth(cmd);
                else if (cmdRegistry.contains(std::string()))
                    cmdHandler = cmdRegistry.fecth(std::string());
                else
                {
                    lgr.error(exceptions::commander::NoSuchCommand(cmd).what());
                    continue;
                }
            }

            if (cmdHandler.first)
                if (pthreadpool)
                    pthreadpool->submit(std::bind(handler_shell, cmdHandler.second, args));
                else
                {
                    lgr.warn(vslz.threadpoolNotDefined.data());
                    handler_shell(cmdHandler.second, args);
                }
            else
                handler_shell(cmdHandler.second, args);
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

    void Command::excute(const std::string& cmdline)
    {
        excute(assistant::split_by(cmdline, " "));
    }

    void Command::excute(const std::list<std::string>& cmdline)
    {
        Command& cmd = get_instance();
        std::unique_lock ul(cmd.cmdQueMutex);
        cmd.cmdQueue.push(cmdline);
        cmd.cmdQueCv.notify_all();
    }

    void Command::sync_excute(const std::string& cmdline)
    {
        sync_excute(assistant::split_by(cmdline, " "));
    }

    void Command::sync_excute(const std::list<std::string>& cmdline)
    {
        if (cmdline.empty())return;
        Command& cmd = get_instance();
        std::function<void(std::list<std::string>)> hdlr;

        std::shared_lock sl(cmd.cmdRegMutex);
        if (cmd.cmdRegistry.contains(cmdline.front()))
            hdlr = cmd.cmdRegistry.fecth(cmdline.front()).second;
        else
            throw exceptions::commander::NoSuchCommand(cmdline.front());
    }

    void Command::async_input::work(background::thread_info& info)
    {
        Command& cmd = Command::get_instance();
        secretary::PrintCenter& pc = secretary::PrintCenter::get_instance();
        while (true)
        {
            pc.set_stick_btm(cmd.vslz.prompt.data());
            pc.flush();
            std::string ln = cmd.sysgetline();

            std::unique_lock ul(inputMutex);
            
            if (waiting > 0)
            {
                line = ln;
                inputCv.notify_all();
            }
            else
                Command::excute(ln);
        }
    }
    std::string Command::async_input::get_line()
    {
        std::unique_lock ul(inputMutex);
        waiting++;
        while(line.empty() && working)
            inputCv.wait(ul);
        waiting--;
        return std::move(line);
    }

    void Command::async_input::stop()
    {
        working = false;
    }

    void Command::async_input::notify_all()
    {
        inputCv.notify_all();
    }
}