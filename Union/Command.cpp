#include "pch.h"
#include "Command.h"

namespace HYDRA15::Union::commander
{
    Command::Command()
    {
        // 重定向输出
        {
            std::lock_guard lg(secretary::PrintCenter::get_instance());
            pCmdOutBuf = new ostreambuf();
            pSysOutBuf = std::cout.rdbuf(pCmdOutBuf);
            std::ostream* psos = pSysOutStream = new std::ostream(pSysOutBuf);
            secretary::PrintCenter::get_instance().redirect([psos](const std::string& str) { *psos << str; });
        }

        // 重定向输入
        {
            secretary::ScanCenter& sc = secretary::ScanCenter::get_instance(true);
            pCmdInBuf = new istreambuf([this]() {return secretary::ScanCenter::getline("cin> "); });
            pSysInBuf = std::cin.rdbuf(pCmdInBuf);
            std::cin.exceptions(std::ios::badbit | std::ios::failbit);
            std::istream* psis = pSysInStream = new std::istream(pSysInBuf);
            sc.set_getline([psis]() { std::string res; std::getline(*psis, res); return res; });
            sc.set_assign([this](const std::string& str) {excute(str); });
            sc.set_defaultPromt(vslz.prompt.data());
            sc.start();
        }
    }

    Command::~Command()
    {
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

    void Command::threadpool(std::shared_ptr<labourer::ThreadLake> p)
    {
        std::unique_lock ul(syslock);
        pthreadpool = p;
    }

    void Command::regist(const std::string& cmd, const command_handler& handler)
    {
        std::unique_lock ul(cmdRegMutex);
        cmdRegistry.regist(cmd, handler);
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

    void Command::regist_command(const std::string& cmd, const command_handler& handler)
    {
        get_instance().regist(cmd, handler);
    }
    void Command::regist_default_command(const command_handler& handler)
    {
        get_instance().regist(std::string(), handler);
    }

    void Command::excute_async(const std::string& cmdline)
    {
        excute_async(assistant::split_by(cmdline, " "));
    }

    void Command::excute_async(const std::list<std::string>& cmdline)
    {
        Command& inst = Command::get_instance();
        std::string cmd = cmdline.front();
        command_handler ch;
        {
            std::shared_lock sl(inst.cmdRegMutex);
            if (inst.cmdRegistry.contains(cmd))
                ch = inst.cmdRegistry.fetch(cmd);
            else if (inst.cmdRegistry.contains(std::string()))
                ch = inst.cmdRegistry.fetch(std::string());
            else throw exceptions::commander::NoSuchCommand(cmd);
        }
        std::shared_lock sl(inst.syslock);
        if (inst.pthreadpool)
            inst.pthreadpool->submit(std::bind(handler_shell, ch, cmdline));
        else
        {
            inst.lgr.debug(vslz.threadpoolNotDefined.data());
            ch(cmdline);
        }
    }

    void Command::excute(const std::string& cmdline)
    {
        excute(assistant::split_by(cmdline, " "));
    }

    void Command::excute(const std::list<std::string>& cmdline)
    {
        if (cmdline.empty())return;
        Command& inst = get_instance();
        command_handler ch;
        std::string cmd = cmdline.front();

        {
            std::shared_lock sl(inst.cmdRegMutex);
            if (inst.cmdRegistry.contains(cmd))
                ch = inst.cmdRegistry.fetch(cmd);
            else if (inst.cmdRegistry.contains(std::string()))
                ch = inst.cmdRegistry.fetch(std::string());
            else
                throw exceptions::commander::NoSuchCommand(cmd);
        }
        ch(cmdline);
    }
}