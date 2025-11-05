#include "pch.h"
#include "single_loader.h"

namespace HYDRA15::Union::archivist
{
    single_loader_v1::head::head(single_loader_v1& ins)
        : ins(ins)
    {
    }

    void single_loader_v1::head::load()
    {
        if (ins.bfs.size() < 96)
            throw exceptions::archivist::FileFormatIncorrect();

        // 读取并验证文件头标记
        std::vector<char> fsign = ins.bfs.read_array<char>(0, 16);
        uint64_t fversion = assistant::byteswap::from_little_endian(ins.bfs.read<uint64_t>(16));
        if (sign != std::string(fsign.data(), 16) || fversion != version)
            throw exceptions::archivist::FileFormatIncorrect();

        // 读取并加载数据
        std::vector<uint64_t> fheader = ins.bfs.read_array<uint64_t>(24, 9);   // 读整个头部数据
        assistant::byteswap::from_little_endian_vector<uint64_t>(fheader);     // 字节序转换

        // 提取数据
        recordCount = fheader[0];
        segSize = fheader[1];
        maxSegCount = fheader[2];
        usedSegCount = fheader[3];
        rootSecSegCount = fheader[4];
        secTabPointer = fheader[5];
        secCount = fheader[6];
        fieldTabPointer = fheader[7];
        fieldCount = fheader[8];
    }

    void single_loader_v1::head::store() const
    {
        std::vector<char> fsign(sign.begin(),sign.end());
        fsign.push_back('\0');

        std::vector<uint64_t> fheader(9, 0);
        fheader[0] = recordCount;
        fheader[1] = segSize;
        fheader[2] = maxSegCount;
        fheader[3] = usedSegCount;
        fheader[4] = rootSecSegCount;
        fheader[5] = secTabPointer;
        fheader[6] = secCount;
        fheader[7] = fieldTabPointer;
        fheader[8] = fieldCount;

        assistant::byteswap::to_little_endian_vector<uint64_t>(fheader);

        ins.bfs.write_array<char>(0, fsign);
        ins.bfs.write_array<uint64_t>(16, fheader);
    }

}
