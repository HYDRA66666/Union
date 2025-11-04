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
    *           16 "ArchivistSingle\0"标记，8 uint版本号(0x0100)，8 表记录条数
    *           8 段大小，8 最大段数，8 已用段数， 8 根节段数
    *           8 节表起始节内指针，8 节表条目数，8 字段表节内起始指针，8 字段表条目数
    *           32N 额外信息
    *           32N 节表记录：
    *               8 节名节内指针，8 起始段号，8 段数，8 已使用大小（字节）
    *           32N 节名数据
    *               节名规则：
    *                    根  节 ：不存储在此处，文件头中已包含
    *                   表记录节：table
    *                   索引表节：index::${indexName}
    *                   数据包节：datapack::${packNO:04X}
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
        static struct visualize
        {
            static_string tableSecName = "table";
            static_string indexSecNameFormat = "index::{}";
            static_string datapackSecFormatName = "datapack::{:04X}";
        }vslz;
        // 根节
        class root_section
        {
        public:
            // 文件开头 64 字节数据
            class header
            {
            public:
                // 应当一致的字段
                static constexpr char flag[16] = "ArchivistSingle";
                static constexpr uint64_t version = 0x00010000;

            public:
                uint64_t dataEntryCount = 0;
                uint64_t segmentSize = 0;
                uint64_t maxSegmentCount = 0;
                uint64_t totalSegmentCount = 0;
                uint64_t rootSectionSegmentCount = 0;
                uint64_t sectionTabPointer = 0;
                uint64_t sectionTabEntryCount = 0;
                uint64_t fieldTabPointer = 0;
                uint64_t fieldTabEntryCount = 0;

            public:
                header() = default;
                bool read(const std::vector<byte>& sectionData);// 从完整的根节数据读取
                bool write(std::vector<byte>& sectionData);     // 将数据写入根节
            }headers;

            // 节表
            class section_table
            {
            public:
                // 节表项
                struct entry
                {
                    uint64_t segStart;
                    uint64_t segCount;
                    uint64_t usedByte;
                };

            public:
                static constexpr size_t sectionEntrySize = 32;

            public:
                std::unordered_map<std::string, entry> sections;

            public:
                section_table() = default;
                bool read(const std::vector<byte>& sectionData, size_t pos, size_t count);
                bool write(std::vector<byte>& sectionData, size_t pos) const; 
                size_t count() const;   // 条目数
                size_t size() const;    // 字节大小
                // 表访问
                auto& at(const std::string&);
                auto at(const std::string&) const;
                auto emplace(const std::string&, const entry&);
                auto contains(const std::string&) const;
            }sections;

            // 字段表
            class field_tab
            {
            public:
                // 两个结构：vector 按顺序存 field_spec ，map 存名称到序号的映射
                std::vector<field_spec> fields;
                std::map<std::string, ID> fieldNameMap;

                field_tab() = default;
                bool read(const std::vector<byte>& sectionData, size_t pos, size_t count);
                bool write(std::vector<byte>& sectionData, size_t pos) const;
                size_t count() const;   // 条目数
                size_t size() const;    // 字节大小
                field_spec fetch(const std::string&) const;
                field_spec fetch(ID) const;
                bool contains(const std::string&) const;
                field_specs field_list() const;
                void store(field_specs);
            }fields;

            root_section() = default;
            bool read(const std::vector<byte>& sectionData);
            bool extract_header(const std::vector<byte>& sectionData);
            void update_header();
            bool write(std::vector<byte>&);
        };
        // 缓存的 datapack 节
        class cached_datapack_seg
        {
            const single_loader_v1& ins;
            uint32_t datapackNo = 0;
            uint64_t segStart = 0;
            uint64_t segCount = 0;
            std::vector<byte> data;
        public:
            cached_datapack_seg(const single_loader_v1& ins);
            std::vector<byte> load_frome_cache(uint32_t dpNo, uint32_t pointer, uint32_t length);
        };
        friend class cached_datapack_seg;


        // 数据
    private:
        random_access_fstream raf;
        root_section rootSection;
        std::mutex mtx;
        // 一些缓存
        const uint64_t segsPerPage; // 每页包含几段
        const ID rowsPerPage;       // 每页包含多少记录
        const size_t bytesPerRow;   // 每条记录32字节对齐后包含多少字节

        // 工具函数
    private:                       
        static uint64_t page_seg_count(size_t entryByteSize, uint64_t segByteSize); // 页大小调优算法
        // 段读写包装
        std::vector<byte> read_segment(uint64_t segID, uint64_t segCount) const;
        bool write_segment(uint64_t segID, std::vector<byte> data);
        // 读写 int 工具
        template<typename I>
            requires std::is_integral_v<I>
        static I read_int(const std::vector<byte>& data, size_t pos)
        {
            I i = 0;
            assistant::memcpy(data.data() + pos, reinterpret_cast<byte*>(&i), sizeof(I));
            i = assistant::byteswap::from_little_endian(i);
            return i;
        }
        template<typename I>
            requires std::is_integral_v<I>
        static void write_int(I i, std::vector<byte>& data, size_t pos)
        {
            i = assistant::byteswap::to_little_endian(i);
            assistant::memcpy(reinterpret_cast<const byte*>(&i), data.data() + pos, sizeof(I));
        }

        // 操作
    private:
        // 节操作
        void move_segments(uint64_t segStart, uint64_t distance, bool forward = false); // 移动位于末尾的一部分段
        void expand_section(const std::string& name, uint64_t newSegCount);             // 将指定节扩展指定段数
        void shrink_section(const std::string& name, uint64_t delSegCOunt);             // 将指定节缩小指定段数
        void new_section(const std::string& name, uint64_t segCount);                   // 创建新节


        // loaader 接口
    public:
        // 信息相关
        virtual size_t size() const override;    // 返回完整的数据大小
        virtual ID tab_size() const override;    // 返回表行数
        virtual ID page_size() const override;   // 返回页大小（以记录数计）

        // 字段信息相关
        virtual field_specs fields() const override;            // 返回完整的字段表

        // 表数据相关
        virtual page rows(ID) const override;   // 返回指定页号的页
        virtual void rows(const page&) override;// 写入整页数据

        // 索引相关
        virtual void index_tab(index) override;                     // 保存索引（包含创建）
        virtual index index_tab(const std::string&) const override; // 加载索引


        // 管理接口
    public:
        single_loader_v1(const std::string& dbFilePath);// 从数据库文件构造
        single_loader_v1(const field_specs& fields, ID preserved,    // 新建文件，需要指定字段、预留的空间（记录数计）
            uint32_t segSize = 4096, uint32_t segMaxCount = std::numeric_limits<uint32_t>::max());  // 可以指定段大小、最大段数

        // 备份相关
        void create_backup(const std::string&) const;   // 创建备份文件，传入文件名
        std::list<std::string> backup_list() const;     // 备份文件列表
        void load_backup(const std::string&);           // 加载备份文件，会覆盖当前表的所有内容
        void delete_backup(const std::string&) const;   // 删除指定备份文件

        // 优化文件
        size_t optimize();
    };

}