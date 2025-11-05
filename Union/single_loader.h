#pragma once
#include "pch.h"
#include "framework.h"

#include "archivist_interfaces.h"
#include "archivist_exception.h"
#include "files.h"
#include "byteswap.h"

namespace HYDRA15::Union::archivist
{
    /********************* 设 计 *********************
    * 将完整的表放在单一文件内存储，文件内包含表数据、数据包、索引
    * 采用不连续的段的形式存储各节数据，数据中所有的指针均为节内指针
    * 段大小一致、仅初始化可设置，可设定最大段数以限制文件大小
    * 不支持多线程操作，任何接口都会自动保证单线程操作
    * 
    * ********************* 文件结构 *********************
    * 根节（第 0 段开始）：
    *   96 文件头：
    *       16 "ArchivistSingle\0"标记，8 版本号，8 记录数
    *       8 段大小，8 最大段数，8 已用段数，8 根节段数
    *       8 节表指针，8节表条目数，8字段表指针，8 字段表条目数
    *   32N 节表：
    *       32N 节表条目：
    *           8 节名指针，8 段编号组指针，8 段编号组条目数，8 保留
    *       32N 节名数据
    *       32N 段编号组数据
    *       - 节名规则：
    *           -  根  节 ：不在此记录
    *           - 表记录节：table
    *           - 表索引节：index::&{indexName}
    *           - 数据包节：packet::&{packetNo:08X}
    *   32N 字段表：
    *       32N 字段表条目：
    *           8 字段名指针，8 字段注释指针，1 字段类型，7 字段标记，8 保留
    *   
    * 表记录节：
    *   32N 表记录条目：
    *       8 数据包编号，8N 字段数据
    *       - 字段数据结构：无数据填全 0 ，INT 和 FLOAT 直接 8 字节存储，数组类型 4 指针 + 4 元素数存储
    * 
    * 表索引节：
    *   8 字段表指针，8 字段数，8 记录指针，8 记录数
    *   8N 字段表：
    *       8 字段名指针（指向根节字段表的字段名）
    *   8N 记录：
    *       8 记录 ID
    * 
    * 数据包节：
    *   8 已用字节，24 保留
    *   8N 连续的数据
    */
    class single_loader_v1 : public loader
    {
    private:    // 一些内部结构
        class head
        {
        public:
            single_loader_v1& ins;
            // 应当一致的内容
            static constexpr std::string_view sign = "ArchivistSingle";
            static constexpr uint64_t version = 0x00010000;
            // 文件头数据
            uint64_t recordCount;
            uint64_t segSize;
            uint64_t maxSegCount;
            uint64_t usedSegCount;
            uint64_t rootSecSegCount;
            uint64_t secTabPointer;
            uint64_t secCount;
            uint64_t fieldTabPointer;
            uint64_t fieldCount;

            head(single_loader_v1& ins);
            void load();
            void store() const;
        };
        friend class head;

        class section
        {
        public:
            single_loader_v1& ins;
            std::deque<uint64_t> segmentList;
            // 随机读写
            template<typename T>
                requires std::is_trivial_v<T>
            std::vector<T> read_array(uint64_t pos, uint64_t count) const;
            template<typename T>
                requires std::is_trivial_v<T>
            void write_array(uint64_t pos, const std::vector<T>& data);
            // 申请新的段
            void expand(uint64_t segCount); // 扩展指定段数
            // 加载与存储
            void load(const std::deque<uint64_t>&);     // 加载段列表（将原有列表替换为参数）
            const std::deque<uint64_t> store() const;   // 存储段列表
            // 构造必须传入引用
            section() = delete;
            section(const section&) = delete;
            section(section&&) = delete;
            section(single_loader_v1&);
        };
        friend class section;

        class section_manager
        {
        public:
            single_loader_v1& ins;
            std::unordered_map<std::string, section> sectionTab;
            void load(uint64_t pos, uint64_t count);
            void store(uint64_t pos);
            section& fetch(const std::string&);
            bool contains(const std::string&);
            section& create(const std::string&);
            section_manager(single_loader_v1&);
        };
        friend class section_manager;

        class field_table
        {
        public:
            single_loader_v1& ins;
            std::unordered_map<std::string, ID> fieldNameTab;
            std::vector<field_spec> fieldTab;
            // 获取
            field_spec fetch(const std::string& name);
            // 加载与存储
            void load(uint64_t pos, uint64_t count);
            void store(uint64_t pos);
            field_table(single_loader_v1&);
        };

    private: // 工具函数

    private: // 数据
        assistant::bfstream bfs;
        head header{ *this };
        section_manager sectionManager{ *this };
        field_table fieldTab{ *this };
        std::mutex mtx;

    public: // loader 接口
        virtual ~single_loader_v1() = default;

        // 信息相关
        virtual size_t size() const override;    // 返回完整的数据大小
        virtual ID tab_size() const override;    // 返回表行数
        virtual ID page_size() const override;   // 返回页大小（以记录数计）
        virtual field_specs fields() const override;             // 返回完整的字段表

        // 表数据相关
        virtual page rows(ID) const override;    // 返回包含指定页号的页
        virtual void rows(const page&) override; // 写入整页数据

        // 索引相关
        virtual void index_tab(index) override;                      // 保存索引表（包含创建）
        virtual index index_tab(const std::string&) const override;  // 加载索引表

    public: // 管理接口
        single_loader_v1(assistant::bfstream&&);        // 外部指定文件
        single_loader_v1(const std::filesystem::path&); // 按照名称打开文件

    public: // 扩展功能
        // 备份相关：备份将以文件的形式呈现，传入完整的文件名
        void create_backup(const std::filesystem::path&);
        void load_bakcup(const std::filesystem::path&);
    };
}