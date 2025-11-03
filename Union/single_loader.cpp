#include "pch.h"
#include "single_loader.h"

namespace HYDRA15::Union::archivist
{
    bool single_loader_v1::header::read(const std::vector<byte>& headerData)
    {
        if (headerData.size() < 64)
            false;
        {   // 检查头部标记和版本号是否对应
            char fFlag[16] = {};
            uint32_t fVersion = 0;
            assistant::memcpy(reinterpret_cast<const char*>(headerData.data()), fFlag, 16);
            assistant::memcpy(headerData.data() + 16, reinterpret_cast<byte*>(&fVersion), 4);
            fVersion = assistant::byteswap::from_little_endian(fVersion);
            if (fVersion != version || std::string(fFlag, 16) != std::string(flag, 16))
                throw exceptions::archivist::FileFormatIncorrect();
        }
        assistant::memcpy(headerData.data() + 20, reinterpret_cast<byte*>(&segmentSize), 4);
        assistant::memcpy(headerData.data() + 24, reinterpret_cast<byte*>(&maxSegmentCount), 4);
        assistant::memcpy(headerData.data() + 28, reinterpret_cast<byte*>(&rootSectionSegmentCount), 4);
        assistant::memcpy(headerData.data() + 32, reinterpret_cast<byte*>(&sectionTabPointer), 4);
        assistant::memcpy(headerData.data() + 36, reinterpret_cast<byte*>(&sectionTabEntryCount), 4);
        assistant::memcpy(headerData.data() + 40, reinterpret_cast<byte*>(&fieldTabPointer), 8);
        assistant::memcpy(headerData.data() + 48, reinterpret_cast<byte*>(&fieldTabEntryCount), 8);
        assistant::memcpy(headerData.data() + 56, reinterpret_cast<byte*>(&dataEntryCount), 8);
        segmentSize = assistant::byteswap::from_little_endian(segmentSize);
        maxSegmentCount = assistant::byteswap::from_little_endian(maxSegmentCount);
        rootSectionSegmentCount = assistant::byteswap::from_little_endian(rootSectionSegmentCount);
        sectionTabPointer = assistant::byteswap::from_little_endian(sectionTabPointer);
        sectionTabEntryCount = assistant::byteswap::from_little_endian(sectionTabEntryCount);
        fieldTabPointer = assistant::byteswap::from_little_endian(fieldTabPointer);
        fieldTabEntryCount = assistant::byteswap::from_little_endian(fieldTabEntryCount);
        dataEntryCount = assistant::byteswap::from_little_endian(dataEntryCount);

        return true;
    }

    bool single_loader_v1::header::write(std::vector<byte>& sectionData)
    {
        if (sectionData.size() < 64)
            return false;
        // 字节序转换
        uint32_t fversion = assistant::byteswap::to_little_endian(version);
        uint32_t fsegmentSize = assistant::byteswap::to_little_endian(segmentSize);
        uint32_t fmaxSegmentCount = assistant::byteswap::to_little_endian(maxSegmentCount);
        uint32_t frootSectionSegmentCount = assistant::byteswap::to_little_endian(rootSectionSegmentCount);
        uint32_t fsectionTabPointer = assistant::byteswap::to_little_endian(sectionTabPointer);
        uint32_t fsectionTabEntryCount = assistant::byteswap::to_little_endian(sectionTabEntryCount);
        uint64_t ffieldTabPointer = assistant::byteswap::to_little_endian(fieldTabPointer);
        uint64_t ffieldTabEntryCount = assistant::byteswap::to_little_endian(fieldTabEntryCount);
        uint64_t fdataEntryCount = assistant::byteswap::to_little_endian(dataEntryCount);

        // 写入数据
        assistant::memcpy(reinterpret_cast<const byte*>(&flag), sectionData.data(), 16);
        assistant::memcpy(reinterpret_cast<const byte*>(&fversion), sectionData.data() + 16, 4);
        assistant::memcpy(reinterpret_cast<const byte*>(&fsegmentSize), sectionData.data() + 20, 4);
        assistant::memcpy(reinterpret_cast<const byte*>(&fmaxSegmentCount), sectionData.data() + 24, 4);
        assistant::memcpy(reinterpret_cast<const byte*>(&frootSectionSegmentCount), sectionData.data() + 28, 4);
        assistant::memcpy(reinterpret_cast<const byte*>(&fsectionTabPointer), sectionData.data() + 32, 4);
        assistant::memcpy(reinterpret_cast<const byte*>(&fsectionTabEntryCount), sectionData.data() + 36, 4);
        assistant::memcpy(reinterpret_cast<const byte*>(&ffieldTabPointer), sectionData.data() + 40, 8);
        assistant::memcpy(reinterpret_cast<const byte*>(&ffieldTabEntryCount), sectionData.data() + 48, 8);
        assistant::memcpy(reinterpret_cast<const byte*>(&fdataEntryCount), sectionData.data() + 56, 8);

        return true;
    }

    bool single_loader_v1::section_table::read(const std::vector<byte>& sectionData, size_t pos, size_t count)
    {
        struct raw_entry
        {
            uint64_t namePointer;
            std::string name;
            entry ety;
        };

        // 提取所有记录数据
        if (sectionData.size() < pos + count * 32)
            throw exceptions::archivist::FileFormatIncorrect();
        std::list<raw_entry> entryList;
        for (size_t i = 0; i < count; i++)
        {
            raw_entry rety;
            assistant::memcpy(sectionData.data() + pos + 32 * i, reinterpret_cast<byte*>(&rety.namePointer), 8);
            assistant::memcpy(sectionData.data() + pos + 32 * i + 8, reinterpret_cast<byte*>(&rety.ety.segStart), 4);
            assistant::memcpy(sectionData.data() + pos + 32 * i + 16, reinterpret_cast<byte*>(&rety.ety.segCount), 4);
            rety.namePointer = assistant::byteswap::from_little_endian(rety.namePointer);
            rety.ety.segStart = assistant::byteswap::from_little_endian(rety.ety.segStart);
            rety.ety.segStart = assistant::byteswap::from_little_endian(rety.ety.segStart);
        }

        // 提取名称并将内容合并到tab中
        for (auto& i : entryList)
        {
            if (sectionData.size() < i.namePointer)
                throw exceptions::archivist::FileFormatIncorrect();
            i.name = std::string(reinterpret_cast<const char*>(sectionData.data() + i.namePointer));
            tab[i.name] = i.ety;
        }

        return true;
    }

    bool single_loader_v1::section_table::write(std::vector<byte>& sectionData, size_t pos)
    {
        // 统计两个block的大小并设置
        size_t resEntrySize = tab.size() * 32;
        size_t resNameSize = 0;
        for (const auto& [k, v] : tab)
            resNameSize += assistant::multiple_m_not_less_than_n(32, k.size() + 1);
        // 检查节空间是否足够
        if (sectionData.size() < pos + resEntrySize + resNameSize)
            return false;

        // 填充数据
        size_t currentEntryPos = pos;               // 当前条目数据填充位置
        size_t currentNamePos = pos + resEntrySize; // 当前名称数据填充的位置
        for (const auto& [k, v] : tab)
        {
            // 写入名称数据
            size_t nameSize = assistant::multiple_m_not_less_than_n(32, k.size() + 1);
            assistant::memcpy(reinterpret_cast<const byte*>(k.data()), sectionData.data() + currentNamePos, k.size());
            assistant::memset<byte>(sectionData.data() + currentNamePos + k.size(), 0, nameSize - k.size());

            // 计算并写入其他数据
            uint64_t namePointer = assistant::byteswap::to_little_endian(pos + resEntrySize + currentNamePos);
            uint32_t segStart = assistant::byteswap::to_little_endian(v.segStart);
            uint32_t segCount = assistant::byteswap::to_little_endian(v.segCount);
            assistant::memcpy(reinterpret_cast<const byte*>(&namePointer), sectionData.data() + currentEntryPos, 8);
            assistant::memcpy(reinterpret_cast<const byte*>(&segStart), sectionData.data() + currentEntryPos + 8, 4);
            assistant::memcpy(reinterpret_cast<const byte*>(&segCount), sectionData.data() + currentEntryPos + 12, 4);

            // 更新当前位置
            currentEntryPos += 32;
            currentNamePos += assistant::multiple_m_not_less_than_n(32, k.size() + 1);
        }

        return true;
    }

}
