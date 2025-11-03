#pragma once
#include "framework.h"
#include "pch.h"


namespace HYDRA15::Union::archivist
{
    /******************************* 设 计 *******************************/
    /* 
    * 数据类型：
    *   支持的六种字段类型：uint, float, uint_array, float_array, bytes, object
    *   标记：I, F, U, D, B, P
    *   在内存中：uint64_t, double, std::vector<uint64_t>, std::vector<double>, std::vector<uint8_t>, std::list<packet>
    *   在文件中：8, 8, 4 pointer + 4 length, 4 pointer + 4 length, 4 pointer + 4 length, 4 pointer + 4 length
    * 
    * 文件结构（单文件、页为单位分区）：
    *   根段：
    *       文件头（第 0 页）：
    *           8 "Archives"字符串标记，4 + 4 uint 版本号，4 页大小，4 总页数，4段表起始指针（本页），4 段表条目数
    *           32N 额外信息
    *           32N 段表：
    *               4 段名指针，4 段起始页号，24 标记
    *               段名规则：
    *                   数据表：/${tabName}/${tabname}.adbt
    *                   索引表：/${tabName}/${indexName}.adbi
    *                   数据包：/${tabName}/${packNo:04X}.apkg
    *           32N 段名数据
    *   表段：
    *       文件头：
    *           8 "Archives"字符串标记，4 + 4 uint 版本号，4 字段表开头指针，4字段数，4 表记录开头指针，4表记录数
    *           32N 额外信息
    *           32N 字段表：
    *               4 字段名指针，1 字段类型，3 标记，24 字段名（如果放得下）
    *           32N 字段名数据
    *           32N 表记录：
    *               4 数据包编号，4 标记，8N 字段数据
    *   数据包段：
    *       连续的数据
    *   索引段：
    *       同表段
    *       字段：
    *           - 主键（条目编号）
    *           - 索引字段 1
    *           - 索引字段 2
    *           - ……
    * 
    * 文件结构（多文件）：
    *   文件名与文件路径：
    *       数据表：/${tabName}/${tabname}.adbt
    *       索引表：/${tabName}/${indexName}.adbi
    *       数据包：/${tabName}/${packNo:04X}.apkg
    *   数据表文件：同 表段
    *   数据包文件：同 数据包段
    *   索引文件：同 索引段
    * 
    * 
    */




    /******************************* 数 据 *******************************/
    // 字段类型：uint, dfloat, uint_array, dfloat_array, bytes, object
    // 内存中的类型：uint64_t, double, std::vector<uint64_t>, std::vector<double>, std::vector<uint8_t>, std::list<packet>
    // 文件中的数据：8 Bytes, 8 Bytes, 4 Bytes pointer + 4 Bytes length, 4 Bytes pointer + 4 Bytes length, 4 Bytes pointer + 4 Bytes length, 4 Bytes pointer + 4 Bytes length

    // 内部使用的数据类型
    using ID = uint32_t;
    using UNIT = uint64_t;
    using BYTE = uint8_t;

    // 六种字段类型
    using NODATA = std::monostate;
    using UINT = uint64_t;
    using FLOAT = double;
    using UINTS = std::vector<UINT>;
    using FLOATS = std::vector<FLOAT>;
    using BYTES = std::vector<BYTE>;
    using OBJECTS = std::list<expressman::packet>;

    // 存储字段数据的容器
    using field = std::variant<NODATA, UINT, FLOAT, UINTS, FLOATS, BYTES, OBJECTS>;

    // 字段信息
    struct field_spec
    {
        // 字段类型枚举
        enum class field_type :char
        {
            NODATA = '0', UINT = 'I', FLOAT = 'F',
            UINTS = 'U', FLOATS = 'D', BYTES = 'B', OBJECTS = 'P'
        };

        ID id;
        std::string name;
        field_type type;
    };

    /******************************* 数据层 *******************************/
    // 数据层：组织存储数据、管理磁盘io
    // 向上提供表访问接口

    // 数据行
    class entry
    {
    public:
        virtual ~entry() = default;

        // 获取、写入记录项
        virtual field at(const field_spec&) = 0;
        virtual entry& set(const field_spec&, const field&) = 0;

        // 信息接口
    public:
        virtual ID id() = 0;

        // 将数据行对象直接作为迭代器使用，需要支持迭代器的操作
    public:
        virtual entry& operator++() = 0;
        virtual std::strong_ordering operator<=>(const entry&) = 0;

        // 行锁
    public:
        virtual void lock() = 0;
        virtual void unlock() = 0;
        virtual bool try_lock() = 0;
    };

    // 事件（数据层，底层加速）
    struct incident
    {
        // 事件类型：增删改查
        enum class incident_type { nothing, create, drop, modify, search };
        // 事件参数
        struct condition_param
        {
            enum class condition_type { nothing, equal, dequal, less, greater };
            field_spec targetField;    // 目标字段
            condition_type type;        // 条件类型：等于，不等于，小于，大于
            field reference;            // 参考值
        };
        using condition_params = std::list<condition_param>;
        struct modify_param
        {
            field_spec targetField;
            field value;
        };
        using modify_params = std::list<modify_param>;
        using incident_param = std::variant<
            std::monostate,                             // 用于无操作
            std::shared_ptr<entry>,                     // 用于 增
            condition_params,                           // 用于 删、查
            std::pair<condition_params,modify_params>   //用于改
        >;

        incident_type type;
        incident_param param;
    };

    // 数据表
    class tablet
    {
    public:
        virtual ~tablet() = default;

        // 获取字段信息接口
    public:
        virtual field_spec get_field_spec(const std::string&) = 0;  // 通过字段名获取

        // 增删改查接口
    public:
        virtual std::shared_ptr<entry> create() = 0;    // 创建新表项，返回新建的表项，从返回的对象向其中写入数据
        virtual void drop(ID) = 0;      // 删除表项
        // 修改和查询集成在一起，查询返回的对象应当能通过其操作原始数据
        virtual std::shared_ptr<entry> at(ID) = 0;                                          // 通过 ID 查询
        virtual std::list<std::shared_ptr<entry>> at(std::function<bool(const entry&)>) = 0;// 通过过滤器查找
        // 执行事件，用于底层加速
        virtual std::list<std::shared_ptr<entry>> excute(const incident&) = 0;

        // 信息和控制接口
    public:
        virtual ID size() = 0;   // 返回表记录条数
        virtual size_t memsize() = 0;   // 返回表所占内存大小

        // 迭代器接口
    public:
        virtual std::shared_ptr<entry> begin() = 0;
        virtual std::shared_ptr<entry> end() = 0;

        // 表锁
    public:
        virtual void lock() = 0;
        virtual void unlock() = 0;
        virtual bool try_lock() = 0;
    };




    /******************************* 事务层 *******************************/
    // 事务层：处理事务、处理算法、管理和操作数据表和索引
    // 向上提供数据库访问接口




    /******************************* 服务层 *******************************/
    // 服务层：将多种协议的访问请求转换为事务层接口调用
    // 对外提供服务接口
}