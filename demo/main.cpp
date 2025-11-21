#include "pch.h"
#include "Union/logger.h"
#include "Union/PrintCenter.h"
#include "Union/ThreadLake.h"
#include "Union/fstreams.h"

using namespace HYDRA15::Union;

secretary::PrintCenter& pc = secretary::PrintCenter::get_instance();



int main()
{
    try
    {
        assistant::bsfstream bsfs{ "test.bin",4096,4 };

        std::vector<int> vec(1024, 0);
        for (int i = 0; i < vec.size(); i++)vec[i] = i;
        std::deque<size_t> ids;
        for (size_t i = 0; i < 1024; i++)ids.push_back(i);
        bsfs.write(ids, 1280, vec);

        auto res = bsfs.read<int>(ids, 1280, 2048);
        for(const auto& i:res)
            pc.println(i);
    }
    catch (const std::exception& e) { pc.println(e.what()); }

}


