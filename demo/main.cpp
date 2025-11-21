#include "pch.h"
#include "Union/logger.h"
#include "Union/PrintCenter.h"
#include "Union/ThreadLake.h"
#include "Union/fstreams.h"
#include "Union/sfstream.h"

using namespace HYDRA15::Union;

secretary::PrintCenter& pc = secretary::PrintCenter::get_instance();



int main()
{
    try
    {
        archivist::sfstream sfs{
            "demo_archive.sfa",
            archivist::sfstream::segment_size_level::I
        };

        std::vector<int> dataToWrite(2048, 0);
        for (size_t i = 0; i < dataToWrite.size(); i++)
            dataToWrite[i] = static_cast<int>(i);

        sfs.write("demo_section", 0, dataToWrite);
        sfs.set_sec_comment("demo_section", "This is a demo section.");
    }
    catch (const std::exception& e) { pc.println(e.what()); }

}


