#pragma once
#include "framework.h"
#include "pch.h"

#include "expressman_interfaces.h"

namespace HYDRA15::Union::archivist::types
{
    // 字段类型：uint, dfloat, uint_array, dfloat_array, bytes, object
    // 内存中的类型：uint64_t, double, std::vector<uint64_t>, std::vector<double>, std::vector<uint8_t>, std::list<packet>
    // 文件中的数据：8 Bytes, 8 Bytes, 4 Bytes pointer + 4 Bytes length, 4 Bytes pointer + 4 Bytes length, 4 Bytes pointer + 4 Bytes length, 4 Bytes pointer + 4 Bytes length

    // 内部使用的数据类型
    using ID = uint32_t;
    using BYTE = uint8_t;

    // 六种字段类型
    using NODATA = std::monostate;
    using UINT = uint64_t;
    using FLOAT = double;
    using UINTS = std::vector<uint64_t>;
    using FLOATS = std::vector<double>;
    using BYTES = std::vector<BYTE>;
    using OBJECTS = std::list<expressman::packet>;

    // 存储字段数据的容器
    using field = std::variant<NODATA, UINT, FLOAT, UINTS, FLOATS, BYTES, OBJECTS>;

    // 字段类型枚举
    enum class type :char
    {
        NODATA = '0', UINT = 'I', FLOAT = 'F',
        UINTS = 'U', FLOATS = 'D', BYTES = 'B', OBJECTS = 'P'
    };

}