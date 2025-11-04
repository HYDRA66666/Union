#include "pch.h"
#include "single_loader.h"

namespace HYDRA15::Union::archivist
{
    bool single_loader_v1::root_section::header::read(const std::vector<byte>& headerData)
    {
        if (headerData.size() < 96)
            false;
        {   // 检查头部标记和版本号是否对应
            char fFlag[16] = {};
            assistant::memcpy(reinterpret_cast<const char*>(headerData.data()), fFlag, 16);
            uint64_t fVersion = read_int<uint64_t>(headerData, 16);
            if (fVersion != version || std::string(fFlag, 16) != std::string(flag, 16))
                throw exceptions::archivist::FileFormatIncorrect();
        }
        dataEntryCount = read_int<uint64_t>(headerData, 24);
        segmentSize = read_int<uint64_t>(headerData, 32);
        maxSegmentCount = read_int<uint64_t>(headerData, 40);
        totalSegmentCount = read_int<uint64_t>(headerData, 48);
        rootSectionSegmentCount = read_int<uint64_t>(headerData, 56);
        sectionTabPointer = read_int<uint64_t>(headerData, 64);
        sectionTabEntryCount = read_int<uint64_t>(headerData, 72);
        fieldTabPointer = read_int<uint64_t>(headerData, 80);
        fieldTabEntryCount = read_int<uint64_t>(headerData, 88);

        return true;
    }

    bool single_loader_v1::root_section::header::write(std::vector<byte>& sectionData)
    {
        if (sectionData.size() < 96)
            return false;

        assistant::memcpy(reinterpret_cast<const byte*>(version), sectionData.data(), 16);
        write_int<uint64_t>(version, sectionData, 16);
        write_int<uint64_t>(dataEntryCount, sectionData, 24);
        write_int<uint64_t>(segmentSize, sectionData, 32);
        write_int<uint64_t>(maxSegmentCount, sectionData, 40);
        write_int<uint64_t>(totalSegmentCount, sectionData, 48);
        write_int<uint64_t>(rootSectionSegmentCount, sectionData, 56);
        write_int<uint64_t>(sectionTabPointer, sectionData, 64);
        write_int<uint64_t>(sectionTabEntryCount, sectionData, 72);
        write_int<uint64_t>(fieldTabPointer, sectionData, 80);
        write_int<uint64_t>(fieldTabEntryCount, sectionData, 88);

        return true;
    }

    bool single_loader_v1::root_section::section_table::read(const std::vector<byte>& sectionData, size_t pos, size_t count)
    {
        struct raw_entry
        {
            uint64_t namePointer = 0;
            std::string name{};
            entry ety{};
        };

        // 提取所有记录数据
        if (sectionData.size() < pos + count * 32)
            throw exceptions::archivist::FileFormatIncorrect();
        std::list<raw_entry> entryList;
        for (size_t i = 0; i < count; i++)
        {
            raw_entry rety;
            rety.namePointer = read_int<uint64_t>(sectionData, pos + 32 * i);
            rety.ety.segStart = read_int<uint64_t>(sectionData, pos + 32 * i + 8);
            rety.ety.segCount = read_int<uint64_t>(sectionData, pos + 32 * i + 16);;
            rety.ety.usedByte = read_int<uint64_t>(sectionData, pos + 32 * i + 24);
            entryList.push_back(rety);
        }

        // 提取名称并将内容合并到tab中
        for (auto& i : entryList)
        {
            if (sectionData.size() < i.namePointer)
                throw exceptions::archivist::FileFormatIncorrect();
            i.name = std::string(reinterpret_cast<const char*>(sectionData.data() + i.namePointer));
            sections[i.name] = i.ety;
        }

        return true;
    }

    bool single_loader_v1::root_section::section_table::write(std::vector<byte>& sectionData, size_t pos) const
    {
        // 统计两个block的大小并设置
        size_t resEntrySize = sections.size() * 32;
        size_t resNameSize = 0;
        for (const auto& [k, v] : sections)
            resNameSize += assistant::multiple_m_not_less_than_n(32, k.size() + 1);
        // 检查节空间是否足够
        if (sectionData.size() < pos + resEntrySize + resNameSize)
            return false;

        // 填充数据
        size_t currentEntryPos = pos;               // 当前条目数据填充位置
        size_t currentNamePos = pos + resEntrySize; // 当前名称数据填充的位置
        for (const auto& [k, v] : sections)
        {
            // 写入名称数据
            size_t nameSize = assistant::multiple_m_not_less_than_n(32, k.size() + 1);
            assistant::memcpy(reinterpret_cast<const byte*>(k.data()), sectionData.data() + currentNamePos, k.size());
            assistant::memset<byte>(sectionData.data() + currentNamePos + k.size(), 0, nameSize - k.size());

            // 计算并写入其他数据
            write_int<uint64_t>(currentNamePos, sectionData, currentEntryPos);
            write_int<uint64_t>(v.segStart, sectionData, currentEntryPos + 8);
            write_int<uint64_t>(v.segCount, sectionData, currentEntryPos + 16);
            write_int<uint64_t>(v.usedByte, sectionData, currentEntryPos + 24);

            // 更新当前位置
            currentEntryPos += 32;
            currentNamePos += nameSize;
        }

        return true;
    }

    size_t single_loader_v1::root_section::section_table::count() const
    {
        return sections.size();
    }

    size_t single_loader_v1::root_section::section_table::size() const
    {
        size_t size = 0;
        for (const auto& [k, v] : sections)
            size += 32 + assistant::multiple_m_not_less_than_n(32, k.size() + 1);
        return size;
    }

    auto single_loader_v1::root_section::section_table::emplace(const std::string& n, const entry& e)
    {
        return sections.emplace(std::pair{ n,e });
    }

    auto single_loader_v1::root_section::section_table::contains(const std::string& n) const
    {
        return sections.contains(n);
    }

    auto& single_loader_v1::root_section::section_table::at(const std::string& name)
    {
        return sections.at(name);
    }

    auto single_loader_v1::root_section::section_table::at(const std::string& name) const
    {
        return sections.at(name);
    }


    bool single_loader_v1::root_section::field_tab::read(const std::vector<byte>& sectionData, size_t pos, size_t count)
    {
        struct raw_field_spec
        {
            uint64_t namePointer = 0;
            char type = 0;
            char mark[7]{};
            uint64_t commentPointer = 0;
        };

        // 读取裸数据
        std::list<raw_field_spec> rfss;
        for (size_t i = 0; i < count; i++)
        {
            raw_field_spec rfs;
            rfs.namePointer = read_int<uint64_t>(sectionData, pos + i * 32);
            assistant::memcpy(sectionData.data() + pos + i * 32 + 8, reinterpret_cast<byte*>(&rfs.type), 1);
            assistant::memcpy(sectionData.data() + pos + i * 32 + 9, reinterpret_cast<byte*>(&rfs.mark), 7);
            rfs.commentPointer = read_int<uint64_t>(sectionData, pos + i * 32 + 16);
            rfss.push_back(rfs);
        }

        // 提取字符串并合并到表中
        fieldNameMap.clear();
        fields.clear();
        fields.resize(count);
        ID i = 0;
        for (const auto& rfs : rfss)
        {
            field_spec fs;
            fs.name = std::string(reinterpret_cast<const char*>(sectionData.data() + rfs.namePointer));
            switch (rfs.type)
            {
                using enum archivist::field_spec::field_type;
            case 'I': fs.type = INT; break;
            case 'F': fs.type = FLOAT; break;
            case 'U': fs.type = INTS; break;
            case 'D': fs.type = FLOATS; break;
            case 'B': fs.type = BYTES; break;
            case 'P': fs.type = OBJECTS; break;
            default: fs.type = NOTHING; break;
            }
            assistant::memcpy(reinterpret_cast<const char*>(rfs.mark), reinterpret_cast<char*>(fs.mark), 7);
            fs.comment = std::string(reinterpret_cast<const char*>(sectionData.data() + rfs.commentPointer));

            fields[i] = fs;
            fieldNameMap[fs.name] = i;
            i++;
        }

        return true;
    }

    bool single_loader_v1::root_section::field_tab::write(std::vector<byte>& sectionData, size_t pos) const
    {
        // 统计两个字段的长度
        size_t resEntrySize = fields.size() * 32;
        size_t resStringSize = 0;
        for (const auto& i : fields)
            resStringSize += assistant::multiple_m_not_less_than_n(32, i.name.size() + 1) + assistant::multiple_m_not_less_than_n(32, i.comment.size() + 1);

        // 检查节空间是否足够
        if (sectionData.size() < pos + resEntrySize + resStringSize)
            return false;

        // 填充数据
        size_t currentEntryPos = pos;
        size_t currentStringPos = pos + resEntrySize;
        for (const auto& i : fields)
        {
            // 写入字符串数据
            size_t nameSize = assistant::multiple_m_not_less_than_n(32, i.name.size() + 1);
            size_t commentSize = assistant::multiple_m_not_less_than_n(32, i.comment.size() + 1);
            assistant::memcpy(reinterpret_cast<const byte*>(i.name.data()), sectionData.data() + currentStringPos, i.name.size());
            assistant::memset<byte>(sectionData.data() + currentStringPos + i.name.size(), 0, nameSize - i.name.size());
            assistant::memcpy(reinterpret_cast<const byte*>(i.comment.data()), sectionData.data() + currentStringPos + nameSize, i.comment.size());
            assistant::memset<byte>(sectionData.data() + currentStringPos + nameSize + i.comment.size(), 0, commentSize - i.comment.size());

            // 写入其他数据
            uint64_t fnamePointer = currentStringPos;
            uint8_t ftype;
            switch (i.type)
            {
                using enum archivist::field_spec::field_type;
            case INT: ftype = 'I'; break;
            case FLOAT: ftype = 'F'; break;
            case INTS: ftype = 'U'; break;
            case FLOATS: ftype = 'D'; break;
            case BYTES: ftype = 'B'; break;
            case OBJECTS: ftype = 'P'; break;
            default: ftype = 0; break;
            }
            uint64_t fcommentPointer = currentStringPos + nameSize;

            write_int<uint64_t>(fnamePointer, sectionData, currentEntryPos);
            assistant::memcpy(reinterpret_cast<const byte*>(&ftype), sectionData.data() + currentEntryPos + 8, 1);
            assistant::memcpy(reinterpret_cast<const byte*>(i.mark), sectionData.data() + currentEntryPos + 9, 7);
            write_int<uint64_t>(fcommentPointer, sectionData, currentEntryPos + 16);
            assistant::memset<byte>(sectionData.data() + currentEntryPos + 24, 0, 8);   // 空余字节填 0

            // 更新当前位置
            currentEntryPos += 32;
            currentStringPos += nameSize + commentSize;
        }

        return true;
    }

    size_t single_loader_v1::root_section::field_tab::count() const
    {
        return fields.size();
    }

    size_t single_loader_v1::root_section::field_tab::size() const
    {
        size_t res = 0;
        for (const auto& i : fields)
            res += 32 + assistant::multiple_m_not_less_than_n(32, i.name.size() + 1) + assistant::multiple_m_not_less_than_n(32, i.comment.size() + 1);
        return res;
    }

    field_spec single_loader_v1::root_section::field_tab::fetch(const std::string& s) const
    {
        return fields.at(fieldNameMap.at(s));
    }

    field_spec single_loader_v1::root_section::field_tab::fetch(ID id) const
    {
        return fields.at(id);
    }

    bool single_loader_v1::root_section::field_tab::contains(const std::string& s) const
    {
        return fieldNameMap.contains(s);
    }

    field_specs single_loader_v1::root_section::field_tab::field_list() const
    {
        field_specs res;
        for (const auto& i : fields)
            res.push_back(i);
        return res;
    }

    void single_loader_v1::root_section::field_tab::store(field_specs lst)
    {
        fields.clear();
        fields.resize(lst.size());
        fieldNameMap.clear();
        ID i = 0;
        for (const auto& f : lst)
        {
            fields[i] = f;
            fieldNameMap[f.name] = i;
            i++;
        }
    }

    bool single_loader_v1::root_section::read(const std::vector<byte>& sectionData)
    {
        size_t rootSecSize = static_cast<size_t>(headers.segmentSize) * static_cast<size_t>(headers.rootSectionSegmentCount);
        if (sectionData.size() < rootSecSize)
            throw exceptions::archivist::FileFormatIncorrect();

        if (!sections.read(sectionData, headers.sectionTabPointer, headers.sectionTabEntryCount))
            return false;
        if (!fields.read(sectionData, headers.fieldTabPointer, headers.fieldTabEntryCount))
            return false;
        return true;
        
    }

    bool single_loader_v1::root_section::extract_header(const std::vector<byte>& sectionData)
    {
        return headers.read(sectionData);
    }

    void single_loader_v1::root_section::update_header()
    {
        headers.sectionTabPointer = 64;
        headers.sectionTabEntryCount = sections.count();
        headers.fieldTabPointer = 64 + sections.size();
        headers.fieldTabEntryCount = fields.count();
    }

    bool single_loader_v1::root_section::write(std::vector<byte>& sectionData)
    {
        if (sectionData.size() < static_cast<size_t>(headers.segmentSize) * static_cast<size_t>(headers.rootSectionSegmentCount))
            return false;
        if (!headers.write(sectionData))
            return false;
        if (!sections.write(sectionData, headers.sectionTabPointer))
            return false;
        if (!fields.write(sectionData, headers.fieldTabPointer))
            return false;
        return true;
    }

    single_loader_v1::cached_datapack_seg::cached_datapack_seg(const single_loader_v1& ins)
        : ins(ins)
    { }

    std::vector<single_loader_v1::byte> single_loader_v1::cached_datapack_seg::load_frome_cache(uint32_t dpNo, uint32_t pointer, uint32_t length)
    {
        uint64_t segSize = ins.rootSection.headers.segmentSize;
        uint32_t reqSegStart = pointer / segSize;
        uint32_t reqSegCount = (pointer + length) / segSize + 1;

        // 从磁盘加载
        if (dpNo == datapackNo)
            if (reqSegStart != segStart || reqSegCount != segCount)
            {
                segStart = reqSegStart;
                segCount = reqSegCount;
                data = ins.read_segment(
                    ins.rootSection.sections.at(std::format(ins.vslz.datapackSecFormatName.data(), datapackNo)).segStart + segStart,
                    segCount
                );
            }
        else
        {
            datapackNo = dpNo;
            segStart = reqSegStart;
            segCount = reqSegCount;
            data = ins.read_segment(
                ins.rootSection.sections.at(std::format(ins.vslz.datapackSecFormatName.data(), datapackNo)).segStart + segStart,
                segCount
            );
        }

        std::vector<byte> res;
        res.resize(length);
        assistant::memcpy(data.data() + pointer - segSize * segStart, res.data(), length);
        return res;
    }



    uint64_t single_loader_v1::page_seg_count(size_t entryByteSize, uint64_t segByteSize)
    {
        // 策略：控制一页在 8 ~ 16 节之间，除非无法满足对齐要求
        size_t pageSize = std::lcm(entryByteSize, static_cast<size_t>(segByteSize));
        uint64_t segCount = pageSize / segByteSize;
        if (segCount < 8)
            segCount *= 16 / segCount;
        return segCount;
    }

    void single_loader_v1::expand_section(const std::string& targetSecName, uint64_t newSegCount)
    {
        root_section::section_table::entry targetSec = rootSection.sections.at(targetSecName);

        uint64_t segCount = rootSection.headers.totalSegmentCount;
        if (rootSection.headers.maxSegmentCount - newSegCount < segCount || segCount + newSegCount > rootSection.headers.maxSegmentCount)
            throw exceptions::archivist::FileFull();

        // 移动段
        move_segments(targetSec.segStart + targetSec.segCount, newSegCount);

        // 改表
        for (auto& [k, v] : rootSection.sections.sections)
            if (v.segStart > targetSec.segStart)
                v.segStart += newSegCount;
        rootSection.headers.totalSegmentCount += newSegCount;
    }

    void single_loader_v1::shrink_section(const std::string& targetSecName, uint64_t delSegCount)
    {
        root_section::section_table::entry targetSec = rootSection.sections.at(targetSecName);

        // 移动段
        move_segments(targetSec.segStart + targetSec.segCount, delSegCount, true);

        // 改表
        for (auto& [k, v] : rootSection.sections.sections)
            if (v.segStart > targetSec.segStart)
                v.segStart -= delSegCount;
        rootSection.headers.totalSegmentCount -= delSegCount;
    }

    void single_loader_v1::new_section(const std::string& name, uint64_t segCount)
    {
        uint64_t totalSegCount = rootSection.headers.totalSegmentCount;
        rootSection.sections.emplace(name, { totalSegCount,segCount,0 });
        std::vector<byte> emptySeg(totalSegCount, 0);
        for (uint64_t i = 0; i < segCount; i++)
            write_segment(i + totalSegCount, emptySeg);
        rootSection.headers.totalSegmentCount += segCount;
    }

    std::vector<single_loader_v1::byte> single_loader_v1::read_segment(uint64_t segID, uint64_t segCount) const
    {
        return raf.read(
            segID * static_cast<size_t>(rootSection.headers.segmentSize),
            rootSection.headers.segmentSize * segCount
        );
    }

    bool single_loader_v1::write_segment(uint64_t segID, std::vector<byte> data)
    {
        return raf.write(
            static_cast<size_t>(segID) * static_cast<size_t>(rootSection.headers.segmentSize),
            data
        );
    }

    void single_loader_v1::move_segments(uint64_t segStart, uint64_t distance, bool forward)
    {
        uint64_t totalSegCount = rootSection.headers.totalSegmentCount;
        std::vector<byte> emptySeg(totalSegCount, 0);
        if (forward)    // 将后面的节向前移动
        {
            for (uint64_t i = segStart; i < totalSegCount; i++)
                write_segment(i - distance, read_segment(i, 1));
            for (uint64_t i = totalSegCount - distance; i < totalSegCount; i++)
                write_segment(i, emptySeg); // 空出来的填 0
        }
        else            // 将节向后面移动
        {
            for (uint64_t i = totalSegCount - 1; i > segStart - 1; i--)
                write_segment(i + distance, read_segment(i, 1));
            for (uint64_t i = segStart + distance - 1; i > segStart - 1; i--)
                write_segment(i, emptySeg);
        }
    }

    size_t single_loader_v1::size() const
    {
        return raf.size();
    }

    ID single_loader_v1::tab_size() const
    {
        return rootSection.headers.dataEntryCount;
    }

    ID single_loader_v1::page_size() const
    {
        return rowsPerPage;
    }

    field_specs single_loader_v1::fields() const
    {
        return rootSection.fields.field_list();
    }



    page single_loader_v1::rows(ID pageNo) const
    {
        if (pageNo >= assistant::multiple_m_not_less_than_n(rowsPerPage, rootSection.headers.dataEntryCount))
            throw exceptions::archivist::FileInvalidAccess();

        std::unique_lock ul{ mtx };

        const size_t fieldCount = rootSection.fields.count();

        std::vector<byte> pageData = read_segment(
            rootSection.sections.at(vslz.tableSecName.data()).segStart + pageNo * segsPerPage,
            segsPerPage
        );  // 裸数据
        page res{
            pageNo,
            pageNo * rowsPerPage,
            pageNo == assistant::multiple_m_not_less_than_n(rowsPerPage, rootSection.headers.dataEntryCount) - 1 ?
                rootSection.headers.dataEntryCount - pageNo * rowsPerPage : rowsPerPage    // 如果是最后一页，则为 总数据数 - 前面的页的数据，否则为 整页数据数
        };
        res.data.resize(rowsPerPage * fieldCount);  // 解析后的数据
        

        cached_datapack_seg cdps(*this);
        for (size_t rowNo = 0; rowNo < res.count; rowNo++)
        {
            uint32_t datapackNo = read_int<uint32_t>(pageData, rowNo * bytesPerRow);
            for (size_t fieldNo = 0; fieldNo < fieldCount; fieldNo++)
            {
                switch (rootSection.fields.fetch(fieldNo).type)
                {
                    using enum field_spec::field_type;
                case NOTHING: continue; break;
                case INT: res.data[rowNo * fieldCount + fieldNo] = read_int<archivist::INT>(pageData, rowNo * bytesPerRow + 8 + fieldNo * 8); break;
                case FLOAT: 
                {
                    archivist::FLOAT t = 0;
                    assistant::memcpy(pageData.data() + rowNo * bytesPerRow + 8 + fieldNo * 8, reinterpret_cast<byte*>(&t), sizeof(archivist::FLOAT));
                    res.data[rowNo * fieldCount + fieldNo] = t;
                    break;
                }
                case INTS:
                {
                    uint32_t pointer = read_int<uint32_t>(pageData, rowNo * bytesPerRow + 8 + fieldNo * 8);
                    uint32_t length = read_int<uint32_t>(pageData, rowNo * bytesPerRow + 8 + fieldNo * 8 + 4);
                    std::vector<byte> data = cdps.load_frome_cache(datapackNo, pointer, length * sizeof(archivist::INT));
                    archivist::INTS intarr;
                    intarr.resize(length);
                    for (size_t i = 0; i < length; i++)
                        intarr[i] = read_int<archivist::INT>(data, i * sizeof(archivist::INT));
                    res.data[rowNo * fieldCount + fieldNo] = intarr;
                    break;
                }
                case FLOATS:
                {
                    uint32_t pointer = read_int<uint32_t>(pageData, rowNo * bytesPerRow + 8 + fieldNo * 8);
                    uint32_t length = read_int<uint32_t>(pageData, rowNo * bytesPerRow + 8 + fieldNo * 8 + 4);
                    std::vector<byte> data = cdps.load_frome_cache(datapackNo, pointer, length * sizeof(archivist::FLOAT));
                    archivist::FLOATS fltarr;
                    fltarr.resize(length);
                    for (size_t i = 0; i < length; i++)
                        assistant::memcpy(
                            data.data() + i * sizeof(archivist::FLOAT),
                            reinterpret_cast<byte*>(&(fltarr[i])),
                            sizeof(archivist::FLOAT)
                        );
                    res.data[rowNo * fieldCount + fieldNo] = fltarr;
                    break;
                }
                case BYTES:
                {
                    uint32_t pointer = read_int<uint32_t>(pageData, rowNo * bytesPerRow + 8 + fieldNo * 8);
                    uint32_t length = read_int<uint32_t>(pageData, rowNo * bytesPerRow + 8 + fieldNo * 8 + 4);
                    std::vector<byte> data = cdps.load_frome_cache(datapackNo, pointer, length * sizeof(archivist::BYTE));
                    archivist::BYTES bytearr;
                    bytearr.resize(length);
                    for (size_t i = 0; i < length; i++)
                        assistant::memcpy(
                            data.data() + i * sizeof(archivist::BYTE),
                            reinterpret_cast<byte*>(&(bytearr[i])),
                            sizeof(archivist::BYTE)
                        );
                    res.data[rowNo * fieldCount + fieldNo] = bytearr;
                    break;
                }
                case OBJECTS:
                {
                    uint32_t pointer = read_int<uint32_t>(pageData, rowNo * bytesPerRow + 8 + fieldNo * 8);
                    uint32_t length = read_int<uint32_t>(pageData, rowNo * bytesPerRow + 8 + fieldNo * 8 + 4);
                    std::vector<byte> data = cdps.load_frome_cache(datapackNo, pointer, length * sizeof(archivist::OBJECT));
                    archivist::OBJECTS objarr;
                    objarr.resize(length);
                    size_t i = 0;
                    for (auto& obj : objarr)
                        assistant::memcpy(
                            data.data() + i * sizeof(archivist::OBJECT),
                            reinterpret_cast<byte*>(&(obj)),
                            sizeof(archivist::OBJECT)
                        ), i++;
                    res.data[rowNo * fieldCount + fieldNo] = objarr;
                    break;
                }
                default: continue; break;
                }
            }
        }

        return res;

    }











}
