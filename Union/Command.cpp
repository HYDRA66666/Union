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
            secretary::PrintCenter::get_instance().redirect([psos](const std::string& str) { *psos << str; });
        }

        // 启动后台线程
        start();
    }

    Command::~Command()
    {
        // 停止后台线程
        working = false;
        wait_for_end();

        // 恢复输出
        {
            std::lock_guard lg(secretary::PrintCenter::get_instance());
            std::cout.rdbuf(pSysOutBuf);
            delete pSysOutStream;
            delete pCmdOutBuf;
            secretary::PrintCenter::get_instance().redirect([](const std::string& str) {std::cout << str; });
        }

        // 按任意键退出
        secretary::PrintCenter::println(vslz.onExit.data());
    }

    Command& Command::get_instance()
    {
        static Command instance;
        return instance;
    }

    void Command::regist(const std::string& cmd, bool async, const command_handler& handler)
    {
        cmdRegistry.regist(cmd, std::pair{ async,handler });
    }

    bool Command::unregist(const std::string& cmd)
    {
        return cmdRegistry.unregist(cmd);
    }

    bool Command::contains(const std::string& cmd) const
    {
        return cmdRegistry.contains(cmd);
    }

    void Command::excute(const std::string& cmdline)
    {
        std::list<std::string> args = assistant::split_by(cmdline, " ");
        std::string cmd = args.front();

        if(!cmdRegistry.contains(cmd))
            throw exceptions::commander::NoSuchCommand(cmdline);

        auto [async, cmdHandler] = cmdRegistry.fecth(cmd);

        if (async)
            threadpool.submit(std::bind(cmdHandler, args));
        else
            cmdHandler(args);
    }

    void Command::work(background::thread_info& info)
    {
        while (working)
        {
            secretary::PrintCenter::get_instance().set_stick_btm(vslz.prompt.data());
            secretary::PrintCenter::get_instance().flush();

            std::cin.getline(inBuf, sizeof(inBuf));
            std::string cmdline = inBuf;
            clear_inbuf();

            if(cmdline.empty())
                continue;

            try
            {
                excute(cmdline);
            }
            catch (const std::exception& e)
            {
                lgr.error(e.what());
            }
            catch (...)
            {
                lgr.error(std::format(vslz.unknownExptDuringExcute.data(), cmdline));
            }
        }
    }

    void Command::clear_inbuf()
    {
        for(auto& c : inBuf)
            c = 0;
    }

    void Command::regist_command(const std::string& cmd, bool async, const command_handler& handler)
    {
        get_instance().regist(cmd, async, handler);
    }
}