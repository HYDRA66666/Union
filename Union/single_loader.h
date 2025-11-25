#pragma once
#include "pch.h"
#include "framework.h"

#include "archivist_interfaces.h"
#include "sfstream.h"

#include "lib_exceptions.h"

namespace HYDRA15::Union::archivist
{
    /***************************** 设 计 ****************************
    * 
    * 依托 sfstream 实现文件分节管理的特性，禁用多线程 IO
    * 固定页大小，页段自动对齐
    * 
    * **************************** 文件格式 ****************************
    * 
    * 自定义头：
    *   16 "ArchivistSingle\0" 标记           8 版本号        8 总记录数
    *   8 字段表指针        8 字段表条目数    8 索引表指针    8 索引表条目数
    *   8 当前数据包编号    24 保留
    *   字段表：
    *       32N 字段记录：
    *           8 字段名指针    8 字段注释指针    1 字段类型    7 标记    8 保留
    *       32N 字段名、字段注释数据
    *   索引表：
    *       32N 索引表记录：
    *           8 索引名指针    8 索引注释指针    8 索引字段 ID 指针    8 索引字段数
    *       32N 索引名、索引注释、索引字段 ID 数据
    * 
    * 表记录节 (table): 
    *   32N 表记录条目: 
    *       4 数据包编号    4 保留    8N 字段数据
    *       - 字段数据结构: 无数据填全 0 , INT 和 FLOAT 8 数据, 数组类型 高 4 指针 + 低 4 长度
    * 
    * 索引节 (index::${indexName})：
    *   32 头：
    *       8 索引记录数    24 保留
    *   8N 索引记录：
    *       8 行号
    * 
    * 数据包节 (data::${packID:08X}):
    *   32 头：
    *       4 已用数据大小    28 保留
    *   连续的数据
    *   
    *
    * ********************************************************************/

    class single_loader : public loader
    {
    private:
        static constexpr std::string_view headerMark{ "ArchivistSingle\0" };
        static constexpr uint64_t headerVersion = 0x00010000;
        static constexpr size_t headerSize = 96;
        static constexpr std::string_view tableSectionName{ "table" };
        static constexpr std::string_view indexSectionFmt{ "index::{}" };
        static constexpr std::string_view dataSectionFmt{ "data::{:08X}" };
        static constexpr uint64_t datPkgDatStartOffset = 32;
        static constexpr uint64_t idxDatStartOffset = 32;

    private:
        sfstream sfs;
        mutable std::shared_mutex smtx;     // 保护实际数据，全局拒绝并发写入

        const field_specs fieldSpecs;
        const ID pageSize;
        const ID rowSize;
        ID totalRecords = 0;
        uint64_t currentPackID = 0;
        std::unordered_map<std::string, index> indexMap;

    private:
        static std::pair<sfstream, field_specs> check_and_extract_field_specs(const std::filesystem::path& p)
        {
            sfstream sfs = sfstream::make(p, 0);
            std::vector<byte> cstHdr = sfs.custom_header();
            uint64_t fieldTabPtr;
            uint64_t fieldEntCnt;
            {   // 检查文件头标记、提取数据
                if (cstHdr.size() < headerSize)
                    throw exceptions::files::FormatNotSupported(p, "Archivist Single database files are expected");

                std::string mark{ reinterpret_cast<char*>(cstHdr.data()),16 };
                uint64_t ver = 0;
                assistant::memcpy(cstHdr.data() + 16, reinterpret_cast<byte*>(&ver), 8);
                ver = assistant::byteswap::from_big_endian(ver);
                if (mark != std::string(headerMark.data(), 16) || ver != headerVersion)
                    throw exceptions::files::FormatNotSupported(p, "Archivist Single database files are expected");

                std::vector<uint64_t> hdrDat(2, 0);
                assistant::memcpy(cstHdr.data() + 32, reinterpret_cast<byte*>(hdrDat.data()), 16);
                assistant::byteswap::from_big_endian_vector(hdrDat);
                fieldTabPtr = hdrDat[0];
                fieldEntCnt = hdrDat[1];
            }

            field_specs fieldSpecs(fieldEntCnt, field_spec{});
            for (uint64_t i = 0; i < fieldEntCnt; i++)
            {
                std::vector<uint64_t> entDat(2, 0);
                assistant::memcpy(cstHdr.data() + fieldTabPtr + 32 * i, reinterpret_cast<byte*>(entDat.data()), 16);
                assistant::byteswap::from_big_endian_vector(entDat);
                uint64_t namePtr = entDat[0];
                uint64_t commPtr = entDat[1];

                field_spec fs{
                    std::string(reinterpret_cast<char*>(cstHdr.data() + namePtr)),
                    std::string(reinterpret_cast<char*>(cstHdr.data() + commPtr)),
                    static_cast<field_spec::field_type>(cstHdr[fieldTabPtr + 32 * i + 16])
                };
                assistant::memcpy(cstHdr.data() + fieldTabPtr + 32 * i + 17, fs.mark, 7);

                fieldSpecs[i] = fs;
            }

            return { sfs,fieldSpecs };
        }

        static ID caculate_page_size(const field_specs& fieldSpecs, uint64_t segSize)
        {   // 行和节以最小公倍数对齐
            uint64_t rowSize = assistant::multiple_m_not_less_than_n(32, (fieldSpecs.size() + 1) * 8);
            uint64_t pageByteSize = std::lcm(rowSize, segSize);
            return pageByteSize / segSize;
        }

        static uint64_t caculate_field_spec_tab_size(const field_specs& fieldSpecs)
        {
            uint64_t size = 0;
            for (const auto& fs : fieldSpecs)
                size += assistant::multiple_m_not_less_than_n(32, fs.name.size() + 1)
                    + assistant::multiple_m_not_less_than_n(32, fs.comment.size() + 1)
                    + 32;
            return size;
        }

        static uint64_t caculate_index_tab_size(const std::unordered_map<std::string, index>& indexMap)
        {
            uint64_t size = 0;
            for (const auto& [k, v] : indexMap)
                size += assistant::multiple_m_not_less_than_n(32, k.size() + 1)
                    + assistant::multiple_m_not_less_than_n(32, v.comment.size() + 1)
                    + assistant::multiple_m_not_less_than_n(32, v.fields.size() * 8)
                    + 32;
            return size;
        }

        void sync_header()
        {
            auto cstHdr = sfs.custom_header();
            uint64_t idxTabPtr, idxEntCnt;
            {
                std::vector<uint64_t> hdrDat(6, 0);
                assistant::memcpy(cstHdr.data() + 24, reinterpret_cast<byte*>(hdrDat.data()), 48);
                assistant::byteswap::from_big_endian_vector(hdrDat);
                totalRecords = hdrDat[0];
                idxTabPtr = hdrDat[3];
                idxEntCnt = hdrDat[4];
                currentPackID = hdrDat[5];
            }

            for (uint64_t i = 0; i < idxEntCnt; i++)
            {
                std::vector<uint64_t> idxEntDat(4, 0);
                assistant::memcpy(cstHdr.data() + idxTabPtr + 32 * i, reinterpret_cast<byte*>(idxEntDat.data()), 32);
                assistant::byteswap::from_big_endian_vector(idxEntDat);

                index idx;
                idx.name = std::string{ reinterpret_cast<char*>(cstHdr.data() + idxEntDat[0]) };
                idx.comment = std::string{ reinterpret_cast<char*>(cstHdr.data() + idxEntDat[1]) };
                idx.fields.reserve(idxEntDat[3]);
                
                std::vector<uint64_t> idxFdIDS(idxEntDat[3], 0);
                assistant::memcpy(cstHdr.data() + idxEntDat[2], reinterpret_cast<byte*>(idxFdIDS.data()), idxEntDat[3] * 8);
                assistant::byteswap::from_big_endian_vector(idxFdIDS);
                for (const auto& id : idxFdIDS)
                    idx.fields.push_back(fieldSpecs[id]);
                
                indexMap[idx.name] = idx;
            }
        }

        void flush_header()
        {
            size_t fieldSpecTabSize = caculate_field_spec_tab_size(fieldSpecs);
            size_t indexTabSize = caculate_index_tab_size(indexMap);
            size_t cstHdrSize = headerSize + fieldSpecTabSize + indexTabSize;
            std::vector<byte> cstHdr(cstHdrSize, 0);

            {   // 写入标记和版本号
                assistant::memcpy(reinterpret_cast<const byte*>(headerMark.data()), cstHdr.data(), 16);
                uint64_t ver = assistant::byteswap::to_big_endian(headerVersion);
                assistant::memcpy(reinterpret_cast<byte*>(&ver), cstHdr.data() + 16, 8);
            }

            {   // 写入基本数据
                std::vector<uint64_t> hdrDat{
                    totalRecords,
                    headerSize, fieldSpecs.size(),
                    headerSize + caculate_field_spec_tab_size(fieldSpecs),indexMap.size(),
                    currentPackID, 0, 0, 0
                };
                assistant::byteswap::to_big_endian_vector(hdrDat);
                assistant::memcpy(reinterpret_cast<const byte*>(hdrDat.data()), cstHdr.data() + 24, headerSize - 24);
            }
            {   // 写入字段表
                uint64_t currentDataPos = headerSize + 32 * fieldSpecs.size();
                for (size_t i = 0; i < fieldSpecs.size(); i++)
                {
                    const auto& fs = fieldSpecs[i];
                    uint64_t nameSize = assistant::multiple_m_not_less_than_n(32, fs.name.size() + 1);
                    uint64_t commSize = assistant::multiple_m_not_less_than_n(32, fs.comment.size() + 1);

                    std::vector<uint64_t> entry(2, 0);
                    std::vector<char> name(nameSize, 0);
                    std::vector<char> comment(commSize, 0);

                    entry[0] = currentDataPos;
                    entry[1] = currentDataPos + nameSize;
                    assistant::byteswap::to_big_endian_vector(entry);

                    assistant::memcpy(fs.name.data(), name.data(), fs.name.size());
                    assistant::memcpy(fs.comment.data(), comment.data(), fs.comment.size());

                    assistant::memcpy(reinterpret_cast<byte*>(entry.data()), cstHdr.data() + headerSize + 32 * i, 16);
                    cstHdr[headerSize + 32 * i + 16] = static_cast<byte>(fs.type);
                    assistant::memcpy(fs.mark, cstHdr.data() + headerSize + 32 * i + 17, 7);

                    assistant::memcpy(reinterpret_cast<byte*>(name.data()), cstHdr.data() + currentDataPos, nameSize);
                    assistant::memcpy(reinterpret_cast<byte*>(comment.data()), cstHdr.data() + currentDataPos + nameSize, commSize);

                    currentDataPos += nameSize + commSize;
                }
            }

            {   // 写入索引表
                uint64_t idxTabPtr = headerSize + fieldSpecTabSize;
                uint64_t currentDataPos = idxTabPtr + 32 * indexMap.size();
                size_t i = 0;
                for (const auto& [k, v] : indexMap)
                {
                    uint64_t nameSize = assistant::multiple_m_not_less_than_n(32, k.size() + 1);
                    uint64_t commSize = assistant::multiple_m_not_less_than_n(32, v.comment.size() + 1);
                    uint64_t idtSize = assistant::multiple_m_not_less_than_n(32, v.fields.size() * 8);

                    std::vector<uint64_t> entry(4, 0);
                    std::vector<char> name(nameSize, 0);
                    std::vector<char> comment(commSize, 0);
                    std::vector<uint64_t> idt(idtSize / 8, 0);

                    entry[0] = currentDataPos;
                    entry[1] = currentDataPos + nameSize;
                    entry[2] = currentDataPos + nameSize + commSize;
                    entry[3] = v.fields.size();
                    assistant::byteswap::to_big_endian_vector(entry);

                    assistant::memcpy(k.data(), name.data(), k.size());
                    assistant::memcpy(v.comment.data(), comment.data(), v.comment.size());

                    for (size_t j = 0; j < v.fields.size(); j++)
                        idt[j] = static_cast<uint64_t>(std::distance(fieldSpecs.begin(),
                            std::find_if(fieldSpecs.begin(), fieldSpecs.end(),
                                [&](const field_spec& fs) { return fs.name == v.fields[j].name; })));
                    assistant::byteswap::to_big_endian_vector(idt);

                    assistant::memcpy(reinterpret_cast<byte*>(entry.data()), cstHdr.data() + idxTabPtr + 32 * i, 32);
                    assistant::memcpy(reinterpret_cast<byte*>(name.data()), cstHdr.data() + currentDataPos, nameSize);
                    assistant::memcpy(reinterpret_cast<byte*>(comment.data()), cstHdr.data() + currentDataPos + nameSize, commSize);
                    assistant::memcpy(reinterpret_cast<byte*>(idt.data()), cstHdr.data() + currentDataPos + nameSize + commSize, idtSize);

                    currentDataPos += nameSize + commSize + idtSize;
                    i++;
                }
            }

            // 文件头落盘
            sfs.custom_header() = cstHdr;
            sfs.flush();
        }

    public:     // 公共接口
        // 信息相关
        virtual size_t size() const override { std::shared_lock sl{ smtx }; return sfs.data().size(); }   // 返回完整的数据大小

        virtual ID tab_size() const override { std::shared_lock sl{ smtx }; return totalRecords; }    // 返回表行数
        virtual ID page_size() const override { return pageSize; }   // 返回页大小（以记录数计）
        virtual field_specs fields() const override { return fieldSpecs; } // 返回完整的字段表

        // 表数据相关
        virtual page rows(ID pageID) const override    // 返回包含指定页号的页
        {
            std::shared_lock sl{ smtx };
            if(pageID * pageSize >= totalRecords)
                throw exceptions::common("Page out of range");

            page pg{ pageID,pageID * pageSize,std::min(pageSize, totalRecords - pageID * pageSize) };
            pg.data.resize(pg.count * fieldSpecs.size(), field{});

            // 读取数据
            for (ID r = 0; r < pg.count; r++)
            {
                std::vector<uint64_t> rowData = sfs.read<uint64_t>(tableSectionName.data(), (pg.start + r) * rowSize, fieldSpecs.size() + 1);
                assistant::byteswap::from_big_endian_vector(rowData);
                for (size_t f = 0; f < fieldSpecs.size(); f++)
                {
                    const auto& fs = fieldSpecs[f];
                    uint64_t datapackNO = rowData[0];
                    switch (fs.type)
                    {
                    case field_spec::field_type::INT:
                    {
                        uint64_t rval = rowData[f + 1];
                        INT val = 0;
                        assistant::memcpy(reinterpret_cast<const byte*>(&rval), reinterpret_cast<byte*>(&val), 8);
                        pg.data[r * fieldSpecs.size() + f] = field{ val };
                        break;
                    }
                    case field_spec::field_type::FLOAT:
                    {
                        uint64_t rval = rowData[f + 1];
                        assistant::memcpy(reinterpret_cast<byte*>(&rval), reinterpret_cast<byte*>(&rval), 8);
                        FLOAT val = 0;
                        assistant::memcpy(reinterpret_cast<const byte*>(&rval), reinterpret_cast<byte*>(&val), sizeof(FLOAT));
                        pg.data[r * fieldSpecs.size() + f] = field{ val };
                        break;
                    }
                    case field_spec::field_type::INTS:
                    {
                        uint64_t rdata = rowData[f + 1];
                        uint64_t dataPtr = (rdata >> 32) & 0xFFFFFFFF;
                        uint64_t dataLen = rdata & 0xFFFFFFFF;

                        INTS vals = sfs.read<INT>(std::format(dataSectionFmt.data(), datapackNO), dataPtr, dataLen);
                        assistant::byteswap::from_big_endian_vector(vals);
                        pg.data[r * fieldSpecs.size() + f] = field{ vals };
                        break;
                    }
                    case field_spec::field_type::FLOATS:
                    {
                        uint64_t rdata = rowData[f + 1];
                        uint64_t dataPtr = (rdata >> 32) & 0xFFFFFFFF;
                        uint64_t dataLen = rdata & 0xFFFFFFFF;

                        std::vector<uint64_t> rvals = sfs.read<uint64_t>(std::format(dataSectionFmt.data(), datapackNO), dataPtr, dataLen);
                        assistant::byteswap::from_big_endian_vector(rvals);
                        FLOATS vals(dataLen, 0);
                        for (size_t i = 0; i < dataLen; i++)
                            assistant::memcpy(reinterpret_cast<const byte*>(&rvals[i]), reinterpret_cast<byte*>(&vals[i]), sizeof(FLOAT));
                        pg.data[r * fieldSpecs.size() + f] = field{ vals };
                        break;
                    }
                    case field_spec::field_type::BYTES:
                    {
                        uint64_t rdata = rowData[f + 1];
                        uint64_t dataPtr = (rdata >> 32) & 0xFFFFFFFF;
                        uint64_t dataLen = rdata & 0xFFFFFFFF;

                        BYTES vals = sfs.read<BYTE>(std::format(dataSectionFmt.data(), datapackNO), dataPtr, dataLen);
                        pg.data[r * fieldSpecs.size() + f] = field{ vals };
                        break;
                    }
                    default:
                        pg.data[r * fieldSpecs.size() + f] = field{ std::monostate{} };
                        break;
                    }
                }
            }
            return pg;
        }

        virtual void rows(const page& pg) override // 写入整页数据
        {
            std::unique_lock ul{ smtx };
            if (pg.start + pg.count > totalRecords)
                totalRecords = pg.start + pg.count;

            // 如果不包含当前数据包节则创建
            if(!sfs.sec_contains(std::format(dataSectionFmt.data(), currentPackID)))
            {
                // 初始化数据包节（将packUsedSize写入节头）
                std::vector<uint32_t> dpHdr(8,0);
                dpHdr[0] = datPkgDatStartOffset;
                assistant::byteswap::to_big_endian_vector(dpHdr);
                sfs.write<uint32_t>(std::format(dataSectionFmt.data(), currentPackID), 0, dpHdr);
            }

            uint32_t currentPackUsedSize = assistant::byteswap::from_big_endian(
                sfs.read<uint32_t>(std::format(dataSectionFmt.data(), currentPackID), 0, 1)[0]);
            for (const auto& rid : pg.modified)
            {
                // 统计此行所需数据包空间（仅包含数组类型）
                uint64_t requiredPackSize = 0;
                for (size_t f = 0; f < fieldSpecs.size(); f++)
                {
                    const auto& fs = fieldSpecs[f];
                    switch (fs.type)
                    {
                    case field_spec::field_type::INTS:
                    {
                        const auto& vals = std::get<INTS>(pg.data[(rid - pg.start) * fieldSpecs.size() + f]);
                        requiredPackSize += vals.size() * 8;
                        break;
                    }
                    case field_spec::field_type::FLOATS:
                    {
                        const auto& vals = std::get<FLOATS>(pg.data[(rid - pg.start) * fieldSpecs.size() + f]);
                        requiredPackSize += vals.size() * 8;
                        break;
                    }
                    case field_spec::field_type::BYTES:
                    {
                        const auto& vals = std::get<BYTES>(pg.data[(rid - pg.start) * fieldSpecs.size() + f]);
                        requiredPackSize += vals.size();
                        break;
                    }
                    default:
                        break;
                    }
                }

                // 如果一行的数据量超过单个数据包容量则报错
                if (requiredPackSize > std::numeric_limits<uint32_t>::max())
                    throw exceptions::common("A single row's data exceeds the capacity of a single data package");

                // 检查数据包空间是否充足，如不充足则创建新数据包
                if (currentPackUsedSize + requiredPackSize > sfs.seg_size() - datPkgDatStartOffset)
                {
                    // 保存旧的数据包已用大小
                    sfs.write<uint32_t>(std::format(dataSectionFmt.data(), currentPackID), 0,
                        std::vector<uint32_t>{ assistant::byteswap::to_big_endian(currentPackUsedSize) });
                    // 切换到新数据包
                    currentPackID++;
                    currentPackUsedSize = datPkgDatStartOffset;
                    // 初始化新数据包节（将packUsedSize写入节头）
                    std::vector<uint32_t> dpHdr(8, 0);
                    dpHdr[0] = datPkgDatStartOffset;
                    assistant::byteswap::to_big_endian_vector(dpHdr);
                    sfs.write<uint32_t>(std::format(dataSectionFmt.data(), currentPackID), 0, dpHdr);
                }

                // 准备行数据
                std::vector<uint64_t> rowData(rowSize / 8, 0);
                rowData[0] = currentPackID;
                for (size_t f = 0; f < fieldSpecs.size(); f++)
                {
                    const auto& fs = fieldSpecs[f];
                    switch (fs.type)
                    {
                    case field_spec::field_type::INT:
                    {
                        INT val = std::get<INT>(pg.data[(rid - pg.start) * fieldSpecs.size() + f]);
                        assistant::memcpy(reinterpret_cast<const byte*>(&val), reinterpret_cast<byte*>(&rowData[f + 1]), sizeof(INT));
                        break;
                    }
                    case field_spec::field_type::FLOAT:
                    {
                        FLOAT val = std::get<FLOAT>(pg.data[(rid - pg.start) * fieldSpecs.size() + f]);
                        assistant::memcpy(reinterpret_cast<const byte*>(&val), reinterpret_cast<byte*>(&rowData[f + 1]), sizeof(FLOAT));
                        break;
                    }
                    case field_spec::field_type::INTS:
                    {
                        const auto& vals = std::get<INTS>(pg.data[(rid - pg.start) * fieldSpecs.size() + f]);
                        // 写入数据包
                        std::vector<INT> beVals = vals;
                        assistant::byteswap::to_big_endian_vector(beVals);
                        sfs.write<INT>(std::format(dataSectionFmt.data(), currentPackID), currentPackUsedSize, beVals);
                        // 更新行数据
                        uint64_t rdata = ((static_cast<uint64_t>(currentPackUsedSize) << 32) & 0xFFFFFFFF00000000) | (vals.size() & 0xFFFFFFFF);
                        assistant::memcpy(reinterpret_cast<const byte*>(&rdata), reinterpret_cast<byte*>(&rowData[f + 1]), sizeof(uint64_t));
                        currentPackUsedSize += vals.size() * 8;
                        break;
                    }
                    case field_spec::field_type::FLOATS:
                    {
                        const auto& vals = std::get<FLOATS>(pg.data[(rid - pg.start) * fieldSpecs.size() + f]);
                        // 写入数据包
                        std::vector<uint64_t> rvals(vals.size(), 0);
                        for (size_t i = 0; i < vals.size(); i++)
                            assistant::memcpy(reinterpret_cast<const byte*>(&vals[i]), reinterpret_cast<byte*>(&rvals[i]), sizeof(FLOAT));
                        assistant::byteswap::to_big_endian_vector(rvals);
                        sfs.write<uint64_t>(std::format(dataSectionFmt.data(), currentPackID), currentPackUsedSize, rvals);
                        // 更新行数据
                        uint64_t rdata = ((static_cast<uint64_t>(currentPackUsedSize) << 32) & 0xFFFFFFFF00000000) | (vals.size() & 0xFFFFFFFF);
                        assistant::memcpy(reinterpret_cast<const byte*>(&rdata), reinterpret_cast<byte*>(&rowData[f + 1]), sizeof(uint64_t));
                        currentPackUsedSize += vals.size() * 8;
                        break;
                    }
                    case field_spec::field_type::BYTES:
                    {
                        const auto& vals = std::get<BYTES>(pg.data[(rid - pg.start) * fieldSpecs.size() + f]);
                        // 写入数据包
                        sfs.write<BYTE>(std::format(dataSectionFmt.data(), currentPackID), currentPackUsedSize, vals);
                        // 更新行数据
                        uint64_t rdata = ((static_cast<uint64_t>(currentPackUsedSize) << 32) & 0xFFFFFFFF00000000) | (vals.size() & 0xFFFFFFFF);
                        assistant::memcpy(reinterpret_cast<const byte*>(&rdata), reinterpret_cast<byte*>(&rowData[f + 1]), sizeof(uint64_t));
                        currentPackUsedSize += vals.size() * 8;
                        break;
                    }
                    default:
                        break;
                    }
                }
                // 写入行数据
                assistant::byteswap::to_big_endian_vector(rowData);
                sfs.write<uint64_t>(tableSectionName.data(), rid * rowSize, rowData);
            }

            // 更新当前数据包已用大小
            sfs.write<uint32_t>(std::format(dataSectionFmt.data(), currentPackID), 0,
                std::vector<uint32_t>{ assistant::byteswap::to_big_endian(currentPackUsedSize) });
        }

        // 索引相关
        virtual void index_tab(index idx) override                      // 保存索引表（包含创建）
        {
            std::unique_lock ul{ smtx };

            // 更新索引元数据
            indexMap[idx.name] = { idx.name,idx.comment,idx.fields };

            // 写入索引数据头
            sfs.write<uint64_t>(std::format(indexSectionFmt.data(), idx.name), 0,
                std::vector<uint64_t>{ assistant::byteswap::to_big_endian(static_cast<uint64_t>(idx.data.size())), 0, 0, 0 });

            // 写入索引数据
            std::vector<ID> beIdxData = idx.data;
            assistant::byteswap::to_big_endian_vector(beIdxData);
            sfs.write<ID>(std::format(indexSectionFmt.data(), idx.name), idxDatStartOffset, beIdxData);
        }

        virtual index index_tab(const std::string& idxName) const override  // 加载索引表
        {
            std::shared_lock sl{ smtx };
            
            // 如果索引不存在则报错
            if (!indexMap.contains(idxName))
                throw exceptions::common("Index not found: " + idxName);

            // 读取索引数据头
            uint64_t idxRecCnt = assistant::byteswap::from_big_endian(
                sfs.read<uint64_t>(std::format(indexSectionFmt.data(), idxName), 0, 1)[0]);

            // 读取并返回数据
            index idx = indexMap.at(idxName);
            idx.data = sfs.read<ID>(std::format(indexSectionFmt.data(), idxName), idxDatStartOffset, idxRecCnt);
            assistant::byteswap::from_big_endian_vector(idx.data);
            return idx;
        }

    public:     // 管理接口
        void flush() { std::unique_lock ul{ smtx }; flush_header(); }

        sfstream& data() { return sfs; }

        const sfstream& data() const { return sfs; }

    private:    // 仅允许工厂构造
        single_loader(const sfstream& sfs, const field_specs& fieldSpecs)
            : sfs(sfs), fieldSpecs(fieldSpecs), pageSize(caculate_page_size(fieldSpecs, sfs.seg_size())),
            rowSize(assistant::multiple_m_not_less_than_n(32, (fieldSpecs.size() + 1) * 8)) {
        }

        single_loader(single_loader&&) = default;

    public:
        single_loader(const single_loader& oth)
            :sfs(oth.sfs), fieldSpecs(oth.fieldSpecs), pageSize(oth.pageSize), rowSize(oth.rowSize),
            totalRecords(oth.totalRecords), indexMap(oth.indexMap), currentPackID(oth.currentPackID) {
        }

        virtual ~single_loader() = default;

    public:     // 工厂方法
        // 方式 1：创建新文件，如果文件已存在则报错
        static single_loader make(
            const std::filesystem::path& p,
            field_specs fieldSpecs,
            sfstream::segment_size_level segSizeLevel = sfstream::segment_size_level::III,
            uint64_t maxSegs = std::numeric_limits<uint64_t>::max()
        ) {
            single_loader sl(sfstream::make(p, segSizeLevel, maxSegs, 0), fieldSpecs);
            sl.flush_header();
            return sl;
        }

        static std::unique_ptr<single_loader> make_unique(
            const std::filesystem::path& p,
            field_specs fieldSpecs,
            sfstream::segment_size_level segSizeLevel = sfstream::segment_size_level::III,
            uint64_t maxSegs = std::numeric_limits<uint64_t>::max()
        ) {
            std::unique_ptr<single_loader> psl{ new single_loader(sfstream::make(p, segSizeLevel, maxSegs, 0), fieldSpecs) };
            psl->flush_header();
            return psl;
        }

        // 方式 2：打开已有的文件
        static single_loader make(const std::filesystem::path& p)
        {
            auto [sfs, fieldSpecs] = check_and_extract_field_specs(p);
            single_loader sl{ sfs,fieldSpecs };
            sl.sync_header();
            return sl;
        }

        static std::unique_ptr<single_loader> make_unique(const std::filesystem::path& p) 
        {
            auto [sfs, fieldSpecs] = check_and_extract_field_specs(p);
            std::unique_ptr<single_loader> psl{ new single_loader(sfs, fieldSpecs) };
            psl->sync_header();
            return psl;
        }
    };

    

}