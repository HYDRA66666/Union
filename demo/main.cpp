#include "pch.h"

#include "Union/Command.h"
using namespace HYDRA15::Union::commander;

int main()
{
    Command& cmd = Command::get_instance();

    cmd.regist("echo", false, [](const std::list<std::string>& args) {
        std::string a = Command::getline("Input something to echo: ");
        std::cout << "Echo: " << a << std::endl;
        });

    cmd.regist("echoasyc", true, [](const std::list<std::string>& args) {
        std::string a = Command::getline("Input something to echo: ");
        std::cout << "Echo: " << a << std::endl;
        });

    cmd.regist("echocin", true, [](const std::list<std::string>& args) {
        std::string a;
        std::cin >> a;
        std::cout << "Echo: " << a << std::endl;
        });

    std::this_thread::sleep_for(std::chrono::seconds(10));

}