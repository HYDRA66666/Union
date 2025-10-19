#pragma once
#include "framework.h"
#include "pch.h"

namespace HYDRA15::Union::archivist
{
    // 字段类型：uint, dfloat, uint_array, dfloat_array, bytes, object
    // 底层类型：uint64_t, double, std::vector<uint64_t>, std::vector<double>, std::vector<uint8_t>, std::list<packet>

    /******************************* 信息数据结构 *******************************/
    // 表头
    struct tablet_header
    {
        uint32_t version[2];
        uint64_t incidentID;
        uint32_t nextDatapack;
        uint8_t marks[20];

        uint32_t entryCount;
    };

    // 字段信息
    struct field_spec
    {
        enum class type : char
        {
            null = 0, uint = 'I', dfloat = 'F',
            uint_array = 'U', dfloat_array = 'D', bytes = 'B', object = 'P'
        };

        std::string fieldName;
        type fieldType;
        uint8_t marks[3];

        uint32_t fieldIdx;
    };

    // 字段表
    using field_tab = std::unordered_map<std::string, field_spec>;

    //条目头
    struct entry_header
    {
        uint32_t datapackNO;
        uint32_t lastverID;
        uint8_t marks[8];
    };


    /******************************* 数据层接口 *******************************/
    // 一条记录
    class entry
    {
    public:
        virtual ~entry() = default;

        // 获取记录信息
        virtual entry_header header() = 0;

        // 获取、写入记录项
        // 所涉及的 std::any 必须存储字段类型对应的底层数据类型
        virtual std::any at(uint32_t fieldID) = 0;
        virtual std::any at(const std::string& fieldName) = 0;
        virtual void set(uint32_t fieldID, const std::any& data) = 0;
        virtual void set(const std::string& fieldName, const std::any& data) = 0;
    };


}