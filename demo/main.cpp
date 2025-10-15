#include "pch.h"

#include "Union/Command.h"
#include "Union/ScanCenter.h"
#include "Union/utility.h"
#include "Union/ThreadLake.h"
using namespace HYDRA15::Union::commander;
using namespace HYDRA15::Union::secretary;
using namespace HYDRA15::Union::assistant;
using namespace HYDRA15::Union::labourer;


int async_work(int a, int b)
{
    std::this_thread::sleep_for(std::chrono::seconds(a));
    return b;
}

void call_back(int a)
{
    std::cout << "Callback catched: " << a << std::endl;
}

int main()
{
    //Command& cmd = Command::get_instance();

    //cmd.regist_command("echo", [](const std::list<std::string>& args) {
    //    std::string a = ScanCenter::getline("Input something to echo: ");
    //    std::cout << "Echo: " << a << std::endl;
    //    });

    //cmd.regist("exit", [](const std::list<std::string>& args) {
    //    std::cout << "exiting..." << std::endl;
    //    exit(0);
    //    });

    //cmd.regist("echocin", [](const std::list<std::string>& args) {
    //    std::string a;
    //    std::cin >> a;
    //    std::cout << "Echo: " << a << std::endl;
    //    });

    ////cmd.regist("", false, [](const std::list<std::string>& args) {
    ////    std::cout << "Unknown command. Type 'echo' or 'echoasyc' or 'echocin'." << std::endl;
    ////    });

    //cmd.excute_sync("echo");

    //std::this_thread::sleep_for(std::chrono::seconds(100));


    ThreadLake tl(4);

    auto ret1 = tl.submit(async_work, 1, 2);
    auto ret2 = tl.submit(std::bind(async_work, 3, 4));
    auto ret3 = tl.submit(std::function<int()>(std::bind(async_work, 5, 6)), std::function<void(int)>(call_back));

    std::cout << ret1.get() << " " << ret2.get() << std::endl;
    
    std::this_thread::sleep_for(std::chrono::seconds(10));
    std::cout << ret3.get() << std::endl;
}
