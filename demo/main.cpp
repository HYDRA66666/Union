#include "pch.h"
#include "Union/logger.h"
#include "Union/PrintCenter.h"

using namespace HYDRA15::Union;

int main()
{
    secretary::log::print = [](const std::string& str) {secretary::PrintCenter::println(str); };
    auto lgr = UNION_CREATE_LOGGER();
    lgr.info("this is a info msg");
    lgr.fatal("fatal of program");

    std::this_thread::sleep_for(std::chrono::seconds(10));
}


