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
        assistant::bsfstream bsfs{ "txt.txt",4096,4 };

        std::vector<byte> vec(1024 * 4096, 0);
        for (auto& i : vec)i = rand();
        std::list<size_t> ids;
        for (size_t i = 0; i < 1025; i++)ids.push_back(i);
        bsfs.write(ids, 256, vec);
    }
    catch (const std::exception& e) { pc.println(e.what()); }
}


