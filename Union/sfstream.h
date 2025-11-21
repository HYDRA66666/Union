#pragma once
#include "pch.h"
#include "framework.h"

#include "fstreams.h"
#include "byteswap.h"
#include "utilities.h"
#include "iMutexies.h"

namespace HYDRA15::Union::archivist
{
    /***************************** 设 计 ****************************
    * 以 段 为文件的最小 IO 单位
    * 用户以 节 为文件分割单位，一个节由数个连续或不连续的段组成
    * 用户访问文件时，只需传入节名、节内偏移量和大小，程序自动管理段节映射
    * 继承自 bsfstream 的特性 ：可选多线程 io 
    *
    * **************************** 文件格式 ****************************
    * 根节（第 0 段开始）：
    *   16 "ArchivistSection" 标记        8 版本号              8 段大小
    *   8 最大段数        8 已用段数      8 根节段表指针        8 根节段数
    *   8 节表起始指针    8 节数          8 自定义头开始指针    8 自定义头长度
    *   32N 根节段表：
    *       8N 段 ID
    *   32N 节表：
    *       8 节名指针    8 节注释指针    8 段表指针    8 段数
    *   32N 节名数据
    *   32N 节注释数据
    *   32N 段表数据
    *   32N 自定义头
    *
    * ********************************************************************/
    class sfstream
    {
    private:
        struct section
        {
            mutable labourer::atomic_shared_mutex asmtx;    // 只保护元数据，不保护实际数据
            std::string comment;
            std::deque<uint64_t> segIDs;
            
        };

    public:
        enum class segment_size_level : size_t {
            I = 4096, II = 64 * 1024, III = 256 * 1024,
            IV = 1 * 1024 * 1024, V = 4 * 1024 * 1024, VI = 64 * 1024 * 1024
        };
        

    private:
        static constexpr std::string_view headerMark{ "ArchivistSection" };
        static constexpr uint64_t headerVersion = 0x00010000;
        static constexpr size_t minFileSize = 96;

    private:
        const std::filesystem::path path;
        assistant::bsfstream bsfs;

        const uint64_t segSize;
        uint64_t maxSegCount;
        std::atomic<uint64_t> usedSegCount;

        section rootSection;
        std::unordered_map<std::string, section> sections;
        std::vector<byte> customHeader;

        mutable labourer::atomic_shared_mutex asmtx;

    private:
        static uint64_t check_and_extract_segSize(const std::filesystem::path& p)
        {
            if (!std::filesystem::exists(p) || !std::filesystem::is_regular_file(p))
                throw exceptions::files::FileNotExist(p);

            assistant::bfstream bfs(p);

            // 检查必要的头大小
            if (bfs.size() < 96)
                throw exceptions::files::FormatNotSupported(p, "Archivist Sectioned files are expected");

            // 检查标记
            std::string mark{ bfs.read<char>(0,16).data(),16 };
            if (mark != headerMark)
                throw exceptions::files::FormatNotSupported(p, "Archivist Sectioned files are expected");

            // 检查版本号
            auto version = bfs.read<uint64_t>(16, 1);
            version[0] = assistant::byteswap::from_big_endian(version[0]);
            if (version[0] != headerVersion)
                throw exceptions::files::FormatNotSupported(p, "the version 0x00010000 is expected");

            // 提取节大小并返回
            auto segSize = bfs.read<uint64_t>(24, 1);
            return assistant::byteswap::from_big_endian(segSize[0]);
        }

        static std::pair<uint64_t, uint64_t> calculate_sec_name_comment_tab_size(const std::unordered_map<std::string, section>& sections)
        {
            uint64_t nameSize = 0;
            uint64_t commSize = 0;
            for (const auto& [k, v] : sections)
            {
                nameSize += assistant::multiple_m_not_less_than_n(32, k.size() + 1);
                commSize += assistant::multiple_m_not_less_than_n(32, v.comment.size() + 1);
            }
            return { nameSize,commSize };
        }

        static uint64_t calculate_sec_tab_size(const std::unordered_map<std::string, section>& sections)
        {
            uint64_t size = 0;
            for (const auto& [k, v] : sections)
                size += assistant::multiple_m_not_less_than_n(32, k.size() + 1)
                + assistant::multiple_m_not_less_than_n(32, v.comment.size() + 1)
                + assistant::multiple_m_not_less_than_n(32, v.segIDs.size() * 8)
                + 32;
            return size;
        }

        void expand(std::deque<uint64_t>& segIDs, uint64_t segCount)    // 将指定节 segIDs 扩展 segCount 个段
        {
            for (uint64_t i = 0; i < segCount; i++)
                segIDs.push_back(usedSegCount.fetch_add(1, std::memory_order::relaxed));
        }



    public:
        template<typename T>
            requires std::is_trivial_v<T>
        std::vector<T> read(const std::string& secName, size_t pos, size_t count) const
        {
            std::shared_lock sl{ asmtx };

            if(!sections.contains(secName))
                throw exceptions::files::ContentNotFound(path)
                .set("content type", "section")
                .set("section name", secName);
            
            const section& sec = sections.at(secName);
            std::shared_lock ssl{ sec.asmtx };
            return bsfs.read<T>(sec.segIDs, pos, count);
        }

        template<typename T>
            requires std::is_trivial_v<T>
        void write(const std::string& secName, size_t pos, const std::vector<T>& data)
        {
            std::shared_lock sl{ asmtx };

            if (!sections.contains(secName))
            {
                labourer::upgrade_lock ugl{ asmtx };
                sections.emplace(std::piecewise_construct, std::forward_as_tuple(secName), std::forward_as_tuple());
            }

            section& sec = sections.at(secName);
            std::shared_lock ssl{ sec.asmtx };
            if (sec.segIDs.size() * segSize < pos + data.size() * sizeof(T))
            {
                labourer::upgrade_lock ugl{ sec.asmtx };
                expand(sec.segIDs, (pos + data.size() * sizeof(T) - sec.segIDs.size() * segSize - 1) / segSize + 1);
            }
            return bsfs.write<T>(sec.segIDs, pos, data);
        }

    public:
        size_t sec_count() const { std::shared_lock sl{ asmtx }; return sections.size(); }

        assistant::bfstream& data() { return bsfs.data(); }

        std::vector<byte>& header() { return customHeader; }

        void flush_rootsec()   // 根节落盘
        {
            std::unique_lock ul{ asmtx };

            {   // 写入标记
                std::vector<char> mark(16, 0);
                assistant::memcpy(headerMark.data(), mark.data(), 16);
                bsfs.write(0, 0, mark);
            }

            // 写入基本数据
            std::vector<uint64_t> basicHeader(10, 0);
            basicHeader[0] = headerVersion;                                                         // 版本号
            basicHeader[1] = segSize;                                                               // 段大小
            basicHeader[2] = maxSegCount;                                                           // 最大段数
            basicHeader[3] = usedSegCount;                                                          // 已用段数
            basicHeader[4] = 96;                                                                    // 根节段表指针
            basicHeader[5] = rootSection.segIDs.size();                                             // 根节段数
            basicHeader[6] = 96 + assistant::multiple_m_not_less_than_n(32, basicHeader[5] * 8);    // 节表指针
            basicHeader[7] = sections.size();                                                       // 节数
            basicHeader[8] = basicHeader[6] + calculate_sec_tab_size(sections);                     // 自定义头指针
            basicHeader[9] = customHeader.size();                                                   // 自定义头大小
            
            {
                auto basicHeaderCpy = basicHeader;
                assistant::byteswap::to_big_endian_vector<uint64_t>(basicHeaderCpy);
                bsfs.write(0, 16, basicHeaderCpy);
            }

            {   // 计算并扩展根节
                uint64_t rootSecTotalSize = basicHeader[8] + assistant::multiple_m_not_less_than_n(32, basicHeader[9]);
                if (rootSecTotalSize > segSize * rootSection.segIDs.size())
                    expand(rootSection.segIDs, (rootSecTotalSize - segSize * rootSection.segIDs.size() - 1) / segSize + 1);
            }

            {   // 写入根节段表
                std::vector<uint64_t> segVec(assistant::multiple_m_not_less_than_n(4, rootSection.segIDs.size()), 0);
                for (size_t i = 0; i < rootSection.segIDs.size(); i++)
                    segVec[i] = rootSection.segIDs[i];
                assistant::byteswap::to_big_endian_vector(segVec);
                bsfs.write(rootSection.segIDs, basicHeader[4], segVec);
            }

            {
                // 写入节表
                uint64_t currentEntryPos = basicHeader[6];
                uint64_t currentNamePos = currentEntryPos + 32 * sections.size();
                uint64_t currentCommPos = currentNamePos + calculate_sec_name_comment_tab_size(sections).first;
                uint64_t currentSegtPos = currentCommPos + calculate_sec_name_comment_tab_size(sections).second;
                for (const auto& [k, v] : sections)
                {
                    uint64_t nameSize = assistant::multiple_m_not_less_than_n(32, k.size() + 1);
                    uint64_t commSize = assistant::multiple_m_not_less_than_n(32, v.comment.size() + 1);
                    uint64_t segtSize = assistant::multiple_m_not_less_than_n(4, v.segIDs.size());

                    std::vector<uint64_t> entry(4, 0);
                    std::vector<char> name(nameSize, 0);
                    std::vector<char> comment(commSize, 0);
                    std::vector<uint64_t> segTab(segtSize, 0);

                    entry[0] = currentNamePos;
                    entry[1] = currentCommPos;
                    entry[2] = currentSegtPos;
                    entry[3] = v.segIDs.size();
                    assistant::byteswap::to_big_endian_vector(entry);

                    assistant::memcpy(k.data(), name.data(), k.size());
                    assistant::memcpy(v.comment.data(), comment.data(), v.comment.size());

                    for (size_t i = 0; i < v.segIDs.size(); i++)
                        segTab[i] = v.segIDs[i];
                    assistant::byteswap::to_big_endian_vector(segTab);

                    bsfs.write(rootSection.segIDs, currentEntryPos, entry);
                    bsfs.write(rootSection.segIDs, currentNamePos, name);
                    bsfs.write(rootSection.segIDs, currentCommPos, comment);
                    bsfs.write(rootSection.segIDs, currentSegtPos, segTab);

                    currentEntryPos += 32;
                    currentNamePos += nameSize;
                    currentCommPos += commSize;
                    currentSegtPos += segtSize;
                }
            }

            {   // 写入自定义头
                bsfs.write(rootSection.segIDs, basicHeader[8], customHeader);
            }
        }

        void sync_rootsec()   // 根节加载
        {
            std::unique_lock ul{ asmtx };

            // 读取基本数据
            auto basicHeader = bsfs.read<uint64_t>(0, 16, 10);
            assistant::byteswap::from_big_endian_vector<uint64_t>(basicHeader);

            uint64_t version = basicHeader[0];          // 版本号
            uint64_t segSizeInFile = basicHeader[1];    // 段大小
            maxSegCount = basicHeader[2];               // 最大段数
            usedSegCount = basicHeader[3];              // 已用段数
            uint64_t rootSegTabPtr = basicHeader[4];    // 根节段表指针
            uint64_t rootSegTabCount = basicHeader[5];  // 根节段数
            uint64_t secTabPtr = basicHeader[6];        // 节表指针
            uint64_t secCount = basicHeader[7];         // 节数
            uint64_t customHeaderPtr = basicHeader[8];  // 自定义头指针
            uint64_t customHeaderSize = basicHeader[9]; // 自定义头大小

            // 读取根节段表
            rootSection.segIDs.clear();
            rootSection.segIDs.push_back(0);
            if (rootSegTabPtr != 0 && rootSegTabCount != 0)
                while (rootSection.segIDs.size() < rootSegTabCount)
                {
                    auto segIDs = bsfs.read<uint64_t>(rootSection.segIDs, rootSegTabPtr, std::min((segSize * rootSection.segIDs.size() - rootSegTabPtr) / 8, rootSegTabCount));
                    assistant::byteswap::from_big_endian_vector<uint64_t>(segIDs);
                    rootSection.segIDs = std::deque<uint64_t>(segIDs.begin(), segIDs.end());
                }

            {   // 读取节表
                sections.clear();
                for (size_t i = 0; i < secCount; i++)
                {
                    auto entry = bsfs.read<uint64_t>(rootSection.segIDs, secTabPtr + 32 * i, 4);
                    assistant::byteswap::from_big_endian_vector<uint64_t>(entry);

                    std::string secName = bsfs.read_string(rootSection.segIDs, entry[0]);
                    std::string secComment = bsfs.read_string(rootSection.segIDs, entry[1]);

                    auto segTabData = bsfs.read<uint64_t>(rootSection.segIDs, entry[2], entry[3]);
                    assistant::byteswap::from_big_endian_vector<uint64_t>(segTabData);
                    std::deque<uint64_t> segIDs{ segTabData.begin(), segTabData.end() };

                    sections[secName].comment = secComment;
                    sections[secName].segIDs = segIDs;
                }
            }

            {   // 读取自定义头
                customHeader = bsfs.read<byte>(rootSection.segIDs, customHeaderPtr, customHeaderSize);
            }
        }

        void set_sec_comment(const std::string& secName, const std::string& comment)
        {
            std::shared_lock sl{ asmtx };
            if (!sections.contains(secName))
                throw exceptions::files::ContentNotFound(path)
                .set("content type", "section")
                .set("section name", secName);
            section& sec = sections.at(secName);
            std::unique_lock ul{ sec.asmtx };
            sec.comment = comment;
        }

        void new_section(const std::string& secName, uint64_t segCount ,const std::string& comment = "")
        {
            std::unique_lock ul{ asmtx };
            if (sections.contains(secName))
                throw exceptions::common::BadParameter("secName", secName, "a unique section name");
            sections[secName].comment = comment;
            expand(sections.at(secName).segIDs, segCount);
        }

    public:
        sfstream() = delete;
        sfstream(const sfstream&) = delete;
        sfstream(sfstream&&) noexcept = default;

        // 构造方式 1：新建文件，原有文件内容将被全部覆盖
        sfstream(
            const std::filesystem::path& p,
            segment_size_level level,
            uint64_t maxSegs = std::numeric_limits<uint64_t>::max(),
            unsigned int ioThreads = 4
        )
            :path(p), segSize(static_cast<size_t>(level)), bsfs(p, static_cast<size_t>(level), ioThreads), maxSegCount(maxSegs)
        {
            if (maxSegCount == 0)throw exceptions::common::BadParameter("maxSegs", "0", "> 0");

            rootSection.segIDs.push_back(0);
            usedSegCount = 1;
            flush_rootsec();
        }

        // 构造方式 2：打开已有文件
        sfstream(const std::filesystem::path& p, unsigned int ioThreads = 4)
            : path(p), segSize(check_and_extract_segSize(p)), bsfs(p, check_and_extract_segSize(p), ioThreads)
        {
            sync_rootsec();
        }

        ~sfstream() { flush_rootsec(); }
        
    };
}