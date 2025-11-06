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

    std::string single_loader_v1::section::read_string(uint64_t pos) const
    {
        uint64_t reqSeg = pos / ins.header.segSize;
        if (reqSeg > segmentList.size())
            throw exceptions::archivist::FileInvalidAccess();
        
        // 缓存命中：
        if (reqSeg >= cache.segStart && reqSeg < cache.segEnd)
            for (size_t p = pos - cache.segStart * ins.header.segSize; p < cache.data.size(); p++)
                if (cache.data[p] == 0) // 缓存中可以找到字符串结尾
                    return std::string(reinterpret_cast<char*>(cache.data.data() + (pos - cache.segStart * ins.header.segSize)));

        // 缓存未命中，二倍速扩大搜索范围
        for (uint64_t rg = 1; ; rg = std::min(rg * 2, segmentList.size() - reqSeg))
        {
            // 加载数据
            std::vector<BYTE> temp(rg * ins.header.segSize, BYTE{});
            for (uint64_t i = reqSeg; i < rg + reqSeg; i++)
                ins.bfs.read(
                    segmentList[i] * ins.header.segSize,
                    ins.header.segSize,
                    temp.data() + (i - reqSeg) * ins.header.segSize
                );
            // 搜索
            for (size_t i = pos - reqSeg * ins.header.segSize; i < temp.size(); i++)
                if (temp[i] == 0)
                    return std::string(reinterpret_cast<char*>(temp.data() + (pos - reqSeg * ins.header.segSize)));
            if (rg + reqSeg >= segmentList.size())
                break;
        }

        // 搜索失败，报错
        throw exceptions::archivist::FileInvalidAccess();
    }

    void single_loader_v1::section::expand(uint64_t segCount)
    {
        if (ins.header.maxSegCount - segCount<ins.header.usedSegCount || ins.header.usedSegCount + segCount>ins.header.maxSegCount)
            throw exceptions::archivist::FileFull();

        for (uint64_t i = 0; i < segCount; i++)
        {
            segmentList.push_back(ins.header.usedSegCount);
            ins.header.usedSegCount++;
        }

        ins.flush_root_sec();
    }

    void single_loader_v1::section::load_cache(uint64_t segMid)
    {
        if (segMid > segmentList.size())
            throw exceptions::archivist::FileInvalidAccess();

        // 计算缓存区间
        uint64_t segEndToCache = segmentList.size() - segMid > cache.preferCacheSegCount / 2 ? segMid + cache.preferCacheSegCount : segmentList.size();
        uint64_t segStartToCache = segMid > cache.preferCacheSegCount / 2 ? segMid - cache.preferCacheSegCount / 2 : 0;

        // 分配空间
        cache.segStart = 0;
        cache.segEnd = 0;
        cache.data.resize((segEndToCache - segStartToCache) * ins.header.segSize, BYTE{});
        // 加载缓存
        for (uint64_t i = segStartToCache; i < segEndToCache; i++)
            ins.bfs.read(
                segmentList[i] * ins.header.segSize,
                ins.header.segSize,
                cache.data.data() + (i - segStartToCache) * ins.header.segSize
            );
        cache.segStart = segStartToCache;
        cache.segEnd = segEndToCache;
    }

    size_t single_loader_v1::section::seg_count() const
    {
        return segmentList.size();
    }

    std::deque<uint64_t>& single_loader_v1::section::access()
    {
        return segmentList;
    }

    void single_loader_v1::section::load(const std::vector<uint64_t>& lst)
    {
        segmentList = std::deque(lst.begin(), lst.end());
        load_cache(0);
    }

    std::vector<uint64_t> single_loader_v1::section::store() const
    {
        return std::vector<uint64_t>(segmentList.begin(),segmentList.end());
    }

    single_loader_v1::section::section(single_loader_v1& ins)
        : ins(ins)
    {

    }

    void single_loader_v1::section_manager::create(const std::string& name)
    {
        sectionTab.emplace(name, ins);
        ins.flush_root_sec();
    }

    single_loader_v1::section& single_loader_v1::section_manager::fetch(const std::string& name)
    {
        return sectionTab.at(name);
    }

    bool single_loader_v1::section_manager::contains(const std::string& name) const
    {
        return sectionTab.contains(name);
    }

    void single_loader_v1::section_manager::remove(const std::string& name)
    {
        sectionTab.erase(name);
        ins.flush_root_sec();
    }

    void single_loader_v1::section_manager::optimize()
    {
        std::vector<std::optional<std::reference_wrapper<uint64_t>>> usedSegMap(ins.header.usedSegCount, std::optional<std::reference_wrapper<uint64_t>>());
        uint64_t realUsedSegCount = 0;

        // 统计使用过的节
        for(auto& [k,v]:sectionTab)
            for (auto& seg : v.access())
            {
                usedSegMap[seg] = std::ref(seg);
                realUsedSegCount++;
            }

        // 移动节
        uint64_t currentSeg = 0;
        for (auto& seg : usedSegMap)
            if (seg.has_value() && seg.value() != currentSeg)
            {
                ins.bfs.write_array<BYTE>(
                    currentSeg * ins.header.segSize,
                    ins.bfs.read_array<BYTE>(seg.value() * ins.header.segSize, ins.header.segSize)
                );
                seg.value() = currentSeg;
                currentSeg++;
            }

        // 更新头数据并修改文件大小
        ins.header.usedSegCount = realUsedSegCount;
        ins.flush_root_sec();
        ins.bfs.resize(realUsedSegCount * ins.header.segSize);
    }

    void single_loader_v1::section_manager::load()
    {
        // 先加载根节
        rootSection.load(ins.bfs.read_array<uint64_t>(96, ins.header.rootSecSegCount));

        // 再加载后续的节
        for (uint64_t i = 0; i < ins.header.secCount; i++)
        {
            auto row = rootSection.read_array<uint64_t>(ins.header.secTabPointer, 4);
            assistant::byteswap::from_little_endian_vector<uint64_t>(row);
            auto segLst = rootSection.read_array<uint64_t>(row[1], row[2]);
            assistant::byteswap::from_little_endian_vector<uint64_t>(segLst);
            std::string secName = rootSection.read_string(row[0]);
            sectionTab.emplace(secName, ins);
            sectionTab[secName].load(segLst);
        }
    }

    void single_loader_v1::section_manager::store()
    {
        // 存储根节
        auto rootSeglstVec = rootSection.store();
        assistant::byteswap::to_little_endian_vector(rootSeglstVec);
        ins.bfs.write_array<uint64_t>(96, rootSeglstVec);

        // 存储其他节
        uint64_t currentEntryPos = ins.header.secTabPointer;
        uint64_t currentNamePos = currentEntryPos + 32 * sectionTab.size();
        uint64_t currentSeglstPos = currentNamePos + [this]() {
            uint64_t nameSize = 0;
            for (const auto& [k, v] : sectionTab)
                nameSize += assistant::multiple_m_not_less_than_n(32, k.size() + 1);
            return nameSize;
            }();
        for (const auto& [k, v] : sectionTab)
        {
            uint64_t nameSize = assistant::multiple_m_not_less_than_n(32, k.size() + 1);
            uint64_t seglstSize = assistant::multiple_m_not_less_than_n(32, v.seg_count() * 8);

            std::vector<uint64_t> row{ currentNamePos,currentSeglstPos,v.seg_count(),0};
            assistant::byteswap::to_little_endian_vector(row);
            std::vector<BYTE> nameVec(nameSize, 0);
            assistant::memcpy(reinterpret_cast<const BYTE*>(k.c_str()), nameVec.data(), k.size() + 1);
            std::vector<uint64_t> seglstVec = v.store();
            seglstVec.resize(seglstSize / 8, uint64_t{});
            assistant::byteswap::to_little_endian_vector(seglstVec);

            rootSection.write_array<uint64_t>(currentEntryPos, row);
            rootSection.write_array<BYTE>(currentNamePos, nameVec);
            rootSection.write_array<uint64_t>(currentSeglstPos, seglstVec);
            
            currentEntryPos += 32;
            currentNamePos += nameSize;
            currentSeglstPos += seglstSize;
        }
    }

    uint64_t single_loader_v1::section_manager::size() const
    {
        uint64_t size = 0;
        for (const auto& [k, v] : sectionTab)
            size += 32 + k.size() + 1 + v.seg_count() * 8;
        return size;
    }

    uint64_t single_loader_v1::section_manager::count() const
    {
        return sectionTab.size();
    }

    single_loader_v1::section_manager::section_manager(single_loader_v1& ins)
        : ins(ins), rootSection(ins)
    {

    }
}
