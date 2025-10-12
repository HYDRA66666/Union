#include "pch.h"

#include "Union/Command.h"
#include "Union/ScanCenter.h"
using namespace HYDRA15::Union::commander;
using namespace HYDRA15::Union::secretary;

int main()
{
    Command& cmd = Command::get_instance();

    cmd.regist_command("echo", [](const std::list<std::string>& args) {
        std::string a = ScanCenter::getline("Input something to echo: ");
        std::cout << "Echo: " << a << std::endl;
        });

    cmd.regist("exit", [](const std::list<std::string>& args) {
        std::cout << "exiting..." << std::endl;
        exit(0);
        });

    cmd.regist("echocin", [](const std::list<std::string>& args) {
        std::string a;
        std::cin >> a;
        std::cout << "Echo: " << a << std::endl;
        });

    //cmd.regist("", false, [](const std::list<std::string>& args) {
    //    std::cout << "Unknown command. Type 'echo' or 'echoasyc' or 'echocin'." << std::endl;
    //    });

    cmd.excute("echo");

    std::this_thread::sleep_for(std::chrono::seconds(100));

}
