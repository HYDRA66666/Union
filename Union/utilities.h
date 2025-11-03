#pragma once
#include "framework.h"
#include "pch.h"

#include "assistant_exception.h"
#include "string_utilities.h"

namespace HYDRA15::Union::assistant
{
    // 输出十六进制的原始数据和对应的ascii字符
    std::string hex_heap(const unsigned char* pBegin, unsigned int size, const std::string& title = "Hex Heap", unsigned int preLine = 32);

    // 将字节转换为十六进制（高效版）
    std::string byte_to_hex(const unsigned char* pBegin, unsigned int size);
    

    // 解析 propreties 文件
    // 强制要求键值分隔符为 = ，unicode字符保持原样，
    std::unordered_map<std::string, std::string> parse_propreties(const std::string& ppts);

    // 解析 csv 文件
    // 支持引号不分割，但是不删除引号
    std::list<std::list<std::string>> parse_csv(const std::string& csv);


    // 内存拷贝
    template<typename T>
    void memcpy(const T* src, T* dest, size_t size)
    {
        for (size_t i = 0; i < size; i++)
            dest[i] = src[i];
    }

    // 内存设置
    template<typename T>
    void memset(T* dest, const T& src, size_t size)
    {
        for (size_t i = 0; i < size; i++)
            dest[i] = src;
    }



    // 打印多个参数到控制台
    template<typename ... Args>
    std::ostream& print(Args ... args)
    {
        return (std::cout << ... << args);
    }


    // 计算不小于某数的倍数
    size_t multiple_m_not_less_than_n(size_t m, size_t n);

}