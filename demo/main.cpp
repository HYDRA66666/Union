//#include "pch.h"
//#include "Union/logger.h"
//#include "Union/PrintCenter.h"
//#include "Union/ThreadLake.h"
//#include "Union/fstreams.h"
//#include "Union/sfstream.h"
//#include "Union/byteswap.h"
//
//using namespace HYDRA15::Union;
//
//secretary::PrintCenter& pc = secretary::PrintCenter::get_instance();
//
//
//
//int main()
//{
//    //archivist::sfstream sfs = archivist::sfstream::make("demo_archive.sfa", archivist::sfstream::segment_size_level::I);
//    //std::vector<int> dataToWrite(2048, 0);
//    //for (size_t i = 0; i < dataToWrite.size(); i++)
//    //    dataToWrite[i] = static_cast<int>(i);
//    //sfs.write("demo_section", 0, dataToWrite);
//    //sfs.set_sec_comment("demo_section", "This is a demo section.");
//
//    //auto psfs = archivist::sfstream::make_unique("demo_archive.sfa");
//    //auto res = psfs->read<int>("demo_section", 0, 2048);
//    //for (auto& i : res)pc.println(i);
//
//    //std::deque<int> dq{ 1,2,3,4,5 };
//
//    //assistant::byteswap::to_big_endian_range(dq);
//
//    //for(const auto& i : dq)
//    //    pc.printf("{:08X}\n", i);
//
//    //char v[16] = "ArchivistSingle";
//    //std::string_view e{ "ArchivistSingle\0" };
//    //std::string c{ v,16 };
//    //pc.println(c == std::string(e.data(),16));
//}


#include "pch.h"

//#include "Union/lib_exceptions.h"
#include "Union/simple_memory_table.h"

using namespace HYDRA15::Union;



int main()
{

}