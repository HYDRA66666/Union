#pragma once
#include "framework.h"
#include "pch.h"

#include "assistant_exception.h"

namespace HYDRA15::Union::assistant
{
    std::string operator*(std::string str, size_t count);

    // 删除头尾的空字符，默认删除所有非可打印字符和空格
    inline std::string strip_front(
        const std::string& str,
        std::function<bool(char)> is_valid = [](char c) {return c > 0x20 && c < 0x7F; }
    );

    inline std::string strip_back(
        const std::string& str,
        std::function<bool(char)> is_valid = [](char c) {return c > 0x20 && c < 0x7F; }
    );

    std::string strip(     
        const std::string& str,
        std::function<bool(char)> is_valid = [](char c) {return c > 0x20 && c < 0x7F; }
    );

    // 删除字符串中所有的空字符，默认删除所有非可打印字符和空格
    std::string strip_all(  
        const std::string& str,
        std::function<bool(char)> is_valid = [](char c) {return c > 0x20 && c < 0x7F; }
    );

    // 删除字符串中的ansi颜色格式
    std::string strip_color(const std::string& str);

    // 检查字符串内容是否全部合法，如不合法则报错
    void check_content(
        const std::string& str,
        std::function<bool(char)> is_valid = [](char c) {return c > 0x20 && c < 0x7F; }
    );

    // 用给定的字符切分字符串
    std::list<std::string> split_by(const std::string& str, const std::string& delimiter = " ");


    //向控制台输出十六进制的原始数据和对应的ascii字符
    std::string hex_heap(const unsigned char* pBegin, unsigned int size, const std::string& title = "Hex Heap", unsigned int preLine = 32);


    // 内存拷贝
    template<typename T>
    void memcpy(const T* src, T* dest, size_t size)
    {
        for (size_t i = 0; i < size; i++)
            dest[i] = src[i];
    }


    // 打印多个参数到控制台
    template<typename ... Args>
    std::ostream& print(Args ... args)
    {
        return (std::cout << ... << args);
    }


}