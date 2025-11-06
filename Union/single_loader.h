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
    *   96 + 32N 文件头：
    *       16 "ArchivistSingle\0"标记，8 版本号，8 记录数
    *       8 段大小，8 最大段数，8 已用段数，8 根节段数
    *       8 节表指针，8节表条目数，8字段表指针，8 字段表条目数
    *       32N 根节段列表：
    *           8N 段号
    *   32N 节表：
    *       32N 节表条目：
    *           8 节名指针，8 段 ID 组指针，8 段 ID 组条目数，8 保留
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
            uint64_t recordCount = 0;
            uint64_t segSize = 0;
            uint64_t maxSegCount = 0;
            uint64_t usedSegCount = 0;
            uint64_t rootSecSegCount = 0;
            uint64_t secTabPointer = 0;
            uint64_t secCount = 0;
            uint64_t fieldTabPointer = 0;
            uint64_t fieldCount = 0;

            head(single_loader_v1& ins);
            void load();
            void store() const;
        };
        friend class head;

        class section
        {
            struct cache
            {
                static constexpr uint64_t preferCacheSegCount = 8;
                uint64_t segStart;  // 含
                uint64_t segEnd;    // 不含
                std::vector<BYTE> data;
            };
        public:
            single_loader_v1& ins;
            std::deque<uint64_t> segmentList;
            mutable cache cache;    // 缓存逻辑：尽量缓存 8 个段，尽量把请求末尾放在缓存中间。只缓存读
            // 随机读写，自动处理段映射和缓存，不负责字节序转换
            template<typename T>
                requires std::is_trivial_v<T>
            std::vector<T> read_array(uint64_t pos, uint64_t count) const;
            template<typename T>
                requires std::is_trivial_v<T>
            void write_array(uint64_t pos, const std::vector<T>& data);
            std::string read_string(uint64_t pos) const;
            // 工具函数
            void expand(uint64_t segCount); // 扩展指定段数
            void load_cache(uint64_t segMid);   // 加载缓存
            // 加载与存储
            void load(const std::vector<uint64_t>&);// 加载段列表（将原有列表替换为参数）
            std::vector<uint64_t> store() const;    // 存储段列表
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
            section rootSection;
            // 存取
            void load();
            void store();
            uint64_t size() const;  // 返回字节大小
            uint64_t count() const; // 返回条目数（不含根节）
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
            void load();
            void store() const;
            uint64_t size() const;  // 返回字节大小
            uint64_t count() const; // 返回条目数
            field_table(single_loader_v1&);
        };

    private: // 工具函数
        // 从字节数据中提取int数据
        template<typename T>
            requires std::is_integral_v<T>
        static std::vector<T> extract_int_array(size_t pos, size_t count, const std::vector<BYTE>& secData);
        template<typename T>
            requires std::is_integral_v<T>
        static void insert_int_array(size_t pos, const std::vector<T>& data, std::vector<BYTE>& secData);

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


    template<typename T>
        requires std::is_trivial_v<T>
    inline std::vector<T> single_loader_v1::section::read_array(uint64_t pos, uint64_t count) const
    {
        uint64_t reqSegStart = pos / ins.header.segSize;
        uint64_t reqSegEnd = (pos + count * sizeof(T)) / ins.header.segSize + 1;
        if (reqSegEnd > segmentList.size())
            throw exceptions::archivist::FileInvalidAccess();

        // 如果缓存命中
        if (reqSegStart >= cache.segStart && reqSegEnd <= cache.segEnd)
        {
            std::vector<T> res(count, T{});
            assistant::memcpy(
                cache.data.data() + (pos - cache.segStart * ins.header.segSize),
                reinterpret_cast<T*>(res.data()),
                count * sizeof(T)
            );
            return res;
        }

        // 加载缓存
        load_cache(reqSegEnd);

        // 准备最终数据
        std::vector<T> res(count, T{});
        uint64_t secPos = pos, resPos = 0;
        // 先从磁盘加载未缓存的部分
        for (uint64_t i = reqSegStart, readed = 0; i < cache.segStart; i++, secPos += readed, resPos += readed)
            ins.bfs.read(
                segmentList[i] * ins.header.segSize + (secPos - i * ins.header.segSize),
                readed = assistant::multiple_m_not_less_than_n(ins.header.segSize, secPos + 1) - secPos,
                reinterpret_cast<BYTE*>(res.data() + resPos)
            );
        // 再从缓存加载缓存的部分
        assistant::memcpy(
            cache.data.data() + (secPos - cache.segStart * ins.header.segSize),
            reinterpret_cast<BYTE*>(res.data() + resPos),
            count * sizeof(T) - resPos
        );
        return res;
    }

    template<typename T>
        requires std::is_trivial_v<T>
    inline void single_loader_v1::section::write_array(uint64_t pos, const std::vector<T>& data)
    {
        uint64_t reqSegStart = pos / ins.header.segSize;
        uint64_t reqSegEnd = (pos + data.size() * sizeof(T)) / ins.header.segSize + 1;
        if (reqSegEnd > segmentList.size()) // 空间不够自动扩容
            expand(segmentList.size() - reqSegEnd);
        for (uint64_t i = reqSegStart, secPos = pos, datPos = 0, writed = 0; i < reqSegEnd; i++, secPos += writed, datPos += writed)
            ins.bfs.write(
                segmentList[i] * ins.header.segSize + (secPos - i * ins.header.segSize),
                writed = assistant::multiple_m_not_less_than_n(ins.header.segSize, secPos + 1) - secPos,
                reinterpret_cast<BYTE*>(data.data() + datPos)
            );
    }

}