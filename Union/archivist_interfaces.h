#pragma once
#include "framework.h"
#include "pch.h"

#include "expressman_interfaces.h"


namespace HYDRA15::Union::archivist
{
    /**************************** 基 础 ****************************/
    // 内部数据类型
    using ID = uint64_t;    // 主键
    using BYTE = uint8_t;   // 字节
    using OBJECT = expressman::packet;

    // 字段数据类型
    using NOTHING = std::monostate;
    using INT = int64_t;
    using FLOAT = double;
    using INTS = std::vector<INT>;
    using FLOATS = std::vector<FLOAT>;
    using BYTES = std::vector<BYTE>;
    using OBJECTS = std::list<OBJECT>;

    // 版本号
    using version_id = ID;

    // 字段
    using field = std::variant<NOTHING, INT, FLOAT, INTS, FLOATS, BYTES, OBJECTS>;

    // 字段信息
    struct field_spec
    {
        // 字段类型枚举
        enum class field_type :char
        {
            NOTHING = 0, INT = 'I', FLOAT = 'F',
            INTS = 'U', FLOATS = 'D', BYTES = 'B', OBJECTS = 'P'
        };

        std::string name{};
        field_type type = field_type::NOTHING;
        uint8_t mark[7]{};
        std::string comment{};
    };
    using field_specs = std::list<field_spec>;

    

    
    /**************************** 持久层 ****************************/
    /*
    * 持久层：负责数据存储，提供表加载相关接口，定义数据在硬盘或其他介质中的布局
    * 分页加载：
    *   仅针对表的数据部分。
    *   页被定义为连续的一部分记录，大小（以记录数计）在加载时确定
    * 索引：
    *   索引在传递时只传递记录的 ID，ID排序按照索引列增序排列
    *   索引一次性加载，不分页
    */

    // 表页
    struct page
    {
        ID no;      // 页号
        ID start;   // 页中首条记录的 ID
        ID count;   // 页内有效记录条数
        std::unordered_set<ID> modified; // 修改过的记录列表
        std::vector<field> data;
    };
    struct index    // 持久层和数据层传递索引的结构
    {
        std::string name;       // 索引表名
        field_specs fields;     // 索引列
        std::vector<ID> data;   // 按照索引顺序排列的记录 ID 数据
    };
    
    class loader
    {
    public:
        virtual ~loader() = default;

        // 信息相关
        virtual size_t size() const = 0;    // 返回完整的数据大小
        virtual ID tab_size() const = 0;    // 返回表行数
        virtual ID page_size() const = 0;   // 返回页大小（以记录数计）
        virtual field_specs fields() const = 0; // 返回完整的字段表

        // 表数据相关
        virtual page rows(ID) const = 0;    // 返回包含指定页号的页
        virtual void rows(const page&) = 0; // 写入整页数据

        // 索引相关
        virtual void index_tab(index) = 0;                      // 保存索引表（包含创建）
        virtual index index_tab(const std::string&) const = 0;  // 加载索引表
    };


    /**************************** 数据层 ****************************/
    /*
    * 数据层：负责数据组织，提供数据表操作相关接口，定义数据在内存中的布局
    * 条目 entry ：
    *   逻辑上即一行记录。
    *   技术上应当类似迭代器，持有一行数据的引用，用户应当可以通过条目对象直接操作表中实际的数据
    * 事件 incident ：
    *   二进制格式封装的增删改查操作，避免接口频繁调用造成的性能浪费，也方便表内部算法优化
    *   执行一系列事件（incidents）时，要求下一个事件都在上一个事件的结果的基础上执行。
    *   如 查 - 查 - 改 系列事件，第一次查询获得结果集 A ，第二次查询在 A 的基础上筛选获得结果集 B ，最后只修改 B 中的记录
    *   执行到分隔事件时，将当前结果暂存，然后以全表为基础执行后续的事件；
    *   执行到联合事件时，将当前结果和暂存的结果结合，然后后续事件在结合的基础上执行
    * 表迭代器和 range ：
    *   数据表支持迭代器，迭代器即 entry 对象本身。
    *   使用迭代器，配合 stl range 相关算法，实现对整表结果的筛选、修改
    */

    class entry
    {
    public:
        virtual ~entry() = default;
        virtual std::unique_ptr<entry> clone() = 0;

        // 记录信息
        virtual bool writable() = 0;// 返回 是否可通过此对象写数据

        // 获取、写入记录项
        virtual field at(const std::string&) const = 0;             // 返回 指定字段的数据
        virtual entry& set(const std::string&, const field&) = 0;   // 写入指定字段

        // 将数据行对象直接作为迭代器使用，需要支持迭代器的操作
        virtual entry& operator++() const = 0;
        virtual entry& operator--() const = 0;
        virtual std::strong_ordering operator<=>(const entry&) const = 0;

        // 行锁
        virtual void lock() = 0;
        virtual void unlock() = 0;
        virtual bool try_lock() = 0;
    };

    struct incident
    {
        // 事件类型
        enum class incident_type { 
            nothing, separate, join, limit, order,  // 无操作，分割，联合，限制条数，排序
            create, drop, modify, search            // 增，删，改，查
        };
        // 事件参数
        struct condition_param  // 条件参数
        {
            enum class condition_type { nothing, equal, dequal, less, greater };
            std::string targetField;// 目标字段
            condition_type type;    // 条件类型：等于，不等于，小于，大于
            field reference;        // 参考值
        };
        struct modify_param     // 修改参数
        {
            std::string targetField;//目标字段
            field value;            // 目标值
        };
        struct order_param    // 排序参数
        {
            std::string targetField;// 目标字段
            bool increaseSeq;       // 是否增序
        };
        using incident_param = std::variant<
            std::monostate,         // 用于无操作
            std::unique_ptr<entry>, // 用于 增
            condition_param,        // 用于 查
            modify_param,           // 用于 改
            ID,                     // 用于限制
            order_param             // 用于排序
        >;

        incident_type type;
        incident_param param;
    };
    using incidents = std::list<incident>;

    class table
    {
    public:
        virtual ~table() = default;

        // 表信息与控制
        virtual ID size() const = 0;        // 返回 记录条数
        virtual ID trim() = 0;              // 优化表记录，返回优化后的记录数
        virtual size_t memsize() const = 0; // 返回 表当前所占内存大小
        virtual size_t memoptimize() = 0;   // 优化内存，返回优化后表所占内存大小
        virtual field_specs field_tab() const = 0;  // 返回完整的字段表
        virtual field_spec get_field_spec(const std::string&) const = 0;// 通过字段名获取指定的字段信息

        // 行访问接口
        virtual std::unique_ptr<entry> create() = 0;                                                // 增：创建一条记录，返回相关的条目对象
        virtual void drop(std::unique_ptr<entry>) = 0;                                              // 删：通过条目对象删除记录
        virtual std::list<std::unique_ptr<entry>> at(const std::function<void(const entry&)>) = 0;  // 改、查：通过过滤器查找记录
        virtual std::list<std::unique_ptr<entry>> excute(const incidents&) = 0;                     // 执行一系列事件，返回执行结果

        // 索引接口
        virtual void create_index(std::string, field_specs) = 0;// 创建指定名称、基于指定字段的索引
        virtual void drop_index(std::string) = 0;               // 删除索引

        // 表锁
    public:
        virtual void lock() = 0;
        virtual void unlock() = 0;
        virtual bool try_lock() = 0;
        virtual void lock_shared() const = 0;
        virtual void unlock_shared() const = 0;
        virtual void try_lock_shared() const = 0;
    };

}