#include "pch.h"
#include "Command.h"

namespace HYDRA15::Union::commander
{
    Command::Command()
    {
        secretary::ScanCenter& sc = secretary::ScanCenter::get_instance();
        sc.set_assign([this](const std::string& str) {excute(str); });
        secretary::log::print = [](const std::string& str) {secretary::PrintCenter::println(str); };
    }

    Command::~Command()
    {


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
        if (cmdMap.contains(cmd))
            throw exceptions::commander::CommandCommandExists(cmd);
        cmdMap.emplace(std::pair<std::string, command_handler>{ cmd, handler });
    }

    bool Command::unregist(const std::string& cmd)
    {
        std::unique_lock ul(cmdRegMutex);
        return cmdMap.erase(cmd);
    }

    bool Command::contains(const std::string& cmd) const
    {
        std::shared_lock sl(cmdRegMutex);
        return cmdMap.contains(cmd);
    }

    Command::command_handler Command::fetch(const std::string& cmd)
    {
        std::list<std::string> args = assistant::split_by(cmd, " ");
        std::shared_lock sl(cmdRegMutex);
        return cmdMap.at(args.front());
    }

    void Command::warp(const command_handler& handler, const std::list<std::string>& args)
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

    void Command::excute(const std::string& cmdline)
    {
        excute(assistant::split_by(cmdline, " "));
    }

    void Command::excute(const std::list<std::string>& cmdline)
    {
        Command& inst = Command::get_instance();
        std::string cmd = cmdline.front();
        command_handler ch;
        {
            std::shared_lock sl(inst.cmdRegMutex);
            if (inst.cmdMap.contains(cmd))
                ch = inst.cmdMap.at(cmd);
            else if (inst.cmdMap.contains(std::string()))
                ch = inst.cmdMap.at(std::string());
            else throw exceptions::commander::NoSuchCommand(cmd);
        }
        std::shared_lock sl(inst.syslock);
        if (inst.pthreadpool)
            inst.pthreadpool->submit(std::function<void()>(std::bind(warp, ch, cmdline)));
        else
        {
            inst.lgr.debug(vslz.threadpoolNotDefined.data());
            std::thread(std::bind(warp, ch, cmdline)).detach();
        }
    }

    void Command::excute_sync(const std::string& cmdline)
    {
        excute_sync(assistant::split_by(cmdline, " "));
    }

    void Command::excute_sync(const std::list<std::string>& cmdline)
    {
        Command& inst = Command::get_instance();
        std::string cmd = cmdline.front();
        command_handler ch;
        {
            std::shared_lock sl(inst.cmdRegMutex);
            if (inst.cmdMap.contains(cmd))
                ch = inst.cmdMap.at(cmd);
            else if (inst.cmdMap.contains(std::string()))
                ch = inst.cmdMap.at(std::string());
            else throw exceptions::commander::NoSuchCommand(cmd);
        }
        ch(cmdline);
    }
}