#pragma once
#include "framework.h"
#include "pch.h"


namespace HYDRA15::Union::archivist
{
    // 表记录
    // 包含记录头和记录数据
    // 记录数据可以为六种类型：整形、浮点、整形数组、浮点数组、字节组、对象
    // 底层类型分别为：uint64, double, std::vector<uint64>. std::vector<double>, std::vector<char>, std::list<expressman::packet>
    // 记录数据由 std::any 存储，记录数据的类型由表头确定
    class entry
    {
        // 表头结构
        struct header
        {
            uint32_t datapackNo;
            uint32_t lastRecordID;
            uint8_t marks[8];
        };

        // 数据
    private:
        header head;
        std::vector<std::any> datas;
    };

}