#pragma once
#include "framework.h"
#include "pch.h"

#include "expressman_interfaces.h"

namespace HYDRA15::Union::archivist
{
    // 字段类型：uint, dfloat, uint_array, dfloat_array, bytes, object
    // 底层类型：uint64_t, double, std::vector<uint64_t>, std::vector<double>, std::vector<uint8_t>, std::list<packet>

    /******************************* 约 定 *******************************/
    // 字段类型：uint, dfloat, uint_array, dfloat_array, bytes, object
    // 内存中的类型：uint64_t, double, std::vector<uint64_t>, std::vector<double>, std::vector<uint8_t>, std::list<packet>
    // 文件中的数据：8 Bytes, 8 Bytes, 4 Bytes pointer + 4 Bytes length, 4 Bytes pointer + 4 Bytes length, 4 Bytes pointer + 4 Bytes length, 4 Bytes pointer + 4 Bytes length

    // 六种字段类型
    using uint = uint64_t;
    using dfloat = double;
    using uint_array = std::vector<uint64_t>;
    using dfloat_array = std::vector<double>;
    using bytes = std::vector<uint8_t>;
    using object = std::list<expressman::packet>;

    // 内部使用的数据类型
    using id = uint32_t;
    using byte = uint8_t;

    /******************************* 数据层接口 *******************************/

    // 数据行
    class entry
    {
    public:
        struct header
        {
            id datapackNO;
            id lastverID;
            byte marks[8];
        };

    public:
        virtual ~entry() = default;

        // 获取、修改记录头
        virtual header get_header() = 0;
        virtual void set_header(const header&) = 0;

        // 获取、写入记录项
        // 所涉及的 std::any 必须存储字段类型对应的底层数据类型
        virtual std::any at(id fieldID) = 0;                    // 通过字段 ID 查询
        virtual std::any at(const std::string& fieldName) = 0;  // 通过字段名查询
        virtual void set(id fieldID, const std::any& data) = 0;
        virtual void set(const std::string& fieldName, const std::any& data) = 0;
    };

    // 数据表
    class tablet
    {
    public:
        // 表头
        struct header
        {
            uint32_t version[2];
            uint64_t incidentID;
            id nextDatapack;
            uint8_t marks[20];

            id entryCount;
        };

        // 字段信息
        struct field
        {
            enum class type : char
            {
                null = 0, uint = 'I', dfloat = 'F',
                uint_array = 'U', dfloat_array = 'D', bytes = 'B', object = 'P'
            };

            std::string fieldName;
            type fieldType;
            byte marks[3];

            id fieldId;
        };
        // 字段表
        using field_tab = std::unordered_map<std::string, field>;

    public:
        virtual ~tablet() = default;

        // 信息获取接口
        virtual header get_header() = 0;
        virtual field get_field() = 0;

        // 增删改查接口
        // 只提供查询接口，查询返回表记录的引用，可以直接通过引用对象修改表记录
        virtual std::shared_ptr<entry> at(id) = 0;                                  // 通过 ID 查询
        virtual std::shared_ptr<entry> at(std::function<bool(const entry&)>) = 0;   // 通过过滤器查找
    };


}