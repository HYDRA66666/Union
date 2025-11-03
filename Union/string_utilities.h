#pragma once
#include "pch.h"
#include "framework.h"

#include "assistant_exception.h"

namespace HYDRA15::Union::assistant
{
    // 将一个字符串重复数遍
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

    // 删除字符串中所有的 ansi 转义串
    // ansi 转义串以 \0x1b 开始，任意字母结束
    std::string strip_ansi_secquence(const std::string& str);

    // 检查字符串内容是否全部合法，如不合法则报错
    void check_content(
        const std::string& str,
        std::function<bool(char)> is_valid = [](char c) {return c > 0x20 && c < 0x7F; }
    );

    // 用给定的字符切分字符串
    std::list<std::string> split_by(const std::string& str, const std::string& delimiter = " ");

    std::list<std::string> split_by(const std::string& str, const std::list<std::string>& deliniters);
}