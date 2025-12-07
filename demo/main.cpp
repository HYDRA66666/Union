#include "pch.h"
#include "Union/iMutexies.h"
#include "Union/PrintCenter.h"

using namespace HYDRA15::Union;


int main()
{
    auto& pc = secretary::PrintCenter::get_instance();
    std::cout << "hello";
}