#pragma once
#include "pch.h"

#include "archivist_interfaces.h"
#include "random_access_fstream.h"
#include "byteswap.h"
#include "utilities.h"

namespace HYDRA15::Union::archivist
{
    /* 设计：
    *   采用单文件模式，数据包、表、索引均存放在一个文件中，一个文件只存放一个表
    *   表的页大小自动调优
    * 
    * 
    *   文件结构 V1：
    *   所有指针、uint均为小端序，所有的字符串必须以\0结尾
    *       根节：
    *           16 "ArchivistSingle\0"标记，4 uint版本号(0x0100)，4 段大小，4 最大段数，4 根节大小（段计）
    *           4 节表起始节内指针，4 节表条目数，8 字段表节内起始指针，8 字段表条目数，8 记录数
    *           32N 额外信息
    *           32N 节表记录：
    *               8 字段名节内指针，4 起始段号，4 段数，16 额外信息
    *           32N 节名数据
    *               节名规则：
    *                    根  节 ：root.adbr
    *                   表记录节：${tabname}.adbt
    *                   索引表节：${indexName}.adbi
    *                   数据包节：${packNo:04X}.adbp
    *           32N 字段表：
    *               8 字段名节内指针，1 字段类型，7 标记，8 字段注释节内指针，8 额外信息
    *           32N 字段名与字段数据
    * 
    *       表记录节：
    *           32N 表记录：
    *               4 数据包编号，4 标记，8N 字段数据
    *               字段数据：
    *                   NOTHING：8 空字节，INT：8 小端序，数据；FLOAT：8 IEEE，数据
    *                   INTS：4 起始指针，4 元素数；FLOATS：4 起始指针，4 元素数
    *                   BYTES：4 起始指针，4 元素数；OBJECT：4 起始指针，4 数据包数
    * 
    *       索引表节：
    *           8 字段表开头节内指针，8 字段数，8 索引记录开头节内指针，8 索引记录数
    *           8N 字段表：
    *               8 字段名根节内指针
    *           8N 索引记录：
    *               8 记录 ID
    * 
    *       数据包节：
    *           连续的数据
    */
    class single_loader_v1 : public loader
    {
    private:
        using byte = random_access_fstream::byte;

        // 一些结构
    private:
        // 文件开头 64 字节数据
        class header
        {
        public:
            // 应当一致的字段
            static constexpr char flag[16] = "ArchivistSingle";
            static constexpr uint32_t version = 0x0100;

        public:
            uint32_t segmentSize = 0;
            uint32_t maxSegmentCount = 0;
            uint32_t rootSectionSegmentCount = 0;
            uint32_t sectionTabPointer = 0;
            uint32_t sectionTabEntryCount = 0;
            uint64_t fieldTabPointer = 0;
            uint64_t fieldTabEntryCount = 0;
            uint64_t dataEntryCount = 0;

        public:
            // 数据读写
            header() = default;
            bool read(const std::vector<byte>& sectionData);// 从完整的根节数据读取
            bool write(std::vector<byte>& sectionData);     // 将数据写入根节
        };

        // 节表
        class section_table
        {
        public:
            // 常量
            static constexpr size_t sectionEntrySize = 32;

            // 节表项
            struct entry
            {
                uint32_t segStart;
                uint32_t segCount;
            };

            // 数据
            std::unordered_map<std::string, entry> tab;

            // 数据读写
            section_table() = default;
            bool read(const std::vector<byte>& sectionData, size_t pos, size_t count);
            bool write(std::vector<byte>& sectionData, size_t pos);
        };

        // 字段表
        class field_tab
        {

        };

        // 根节
        class root_section
        {

        };




    };

}