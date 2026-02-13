import std;
import HYDRA15.Union.ThreadLake;
import HYDRA15.Union.log;
import HYDRA15.Union.PrintCenter;

using namespace HYDRA15::Union;

int main()
{
    log::print = [](const std::string& str) { PrintCenter::println(str); };
    ThreadLake lake(4, "TestLake");
    lake.submit([]() {
        log::info("ThreadLake", "Hello from the ThreadLake!");
    });
}
