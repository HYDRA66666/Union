#pragma once
#include "pch.h"
#include "framework.h"

namespace HYDRA15::Union::assistant
{
    // 将一个字符串重复数遍
    inline std::string operator*(std::string str, size_t count)
    {
        std::string result;
        result.reserve(str.size() * count);
        for (size_t i = 0; i < count; ++i) {
            result += str;
        }
        return result;
    }

    // 删除头尾的空字符，默认删除所有非可打印字符和空格
    inline std::string strip_front(
        const std::string& str,
        std::function<bool(char)> is_valid = [](char c) {return c > 0x20 && c < 0x7F; }
    ) {
        std::string res = str;
        size_t pos = 0;
        while (pos < res.size())
        {
            if (is_valid(res[pos]))
                break;
            pos++;
        }
        res.erase(0, pos);
        return res;
    }

    inline std::string strip_back(
        const std::string& str,
        std::function<bool(char)> is_valid = [](char c) {return c > 0x20 && c < 0x7F; }
    ) {
        std::string res = str;
        size_t pos = res.size() - 1;
        while (pos > 0)
        {
            if (is_valid(res[pos]))
                break;
            pos--;
        }
        res.erase(pos + 1);
        return res;
    }

    inline std::string strip(
        const std::string& str,
        std::function<bool(char)> is_valid = [](char c) {return c > 0x20 && c < 0x7F; }
    ) {
        return strip_back(strip_front(str, is_valid), is_valid);
    }

    // 删除字符串中所有的空字符，默认删除所有非可打印字符和空格
    inline std::string strip_all(
        const std::string& str,
        std::function<bool(char)> is_valid = [](char c) {return c > 0x20 && c < 0x7F; }
    ) {
        std::string res;
        res.reserve(str.size());

        for (char c : str)
            if (is_valid(c))
                res.push_back(c);

        res.shrink_to_fit();
        return res;
    }

    // 删除字符串中的ansi颜色格式
    inline std::string strip_color(const std::string& str)
    {
        std::string res;
        res.reserve(str.size());

        for (size_t i = 0; i < str.size(); i++)
        {
            if (str[i] == '\033' && i + 1 < str.size() && str[i + 1] == '[') // 发现 ANSI 转义序列起始
            {
                // 查找 'm' 结尾
                size_t j = i + 2;
                while (j < str.size() && str[j] != 'm')
                    j++;
                if (j < str.size() && str[j] == 'm') {
                    // 找到完整的 ANSI 颜色代码，跳过整个序列 (从 \033 到 m)
                    i = j;
                    continue;
                }
            }
            // 将可保留的字符复制到 res 中
            res.push_back(str[i]);
        }

        return res;
    }

    // 删除字符串中所有的 ansi 转义串
    // ansi 转义串以 \0x1b 开始，任意字母结束
    inline std::string strip_ansi_secquence(const std::string& str)
    {
        if (str.empty())return std::string();

        auto is_charactor = [](char c) {return (c >= 0x41 && c <= 0x5A) || (c >= 0x61 && c <= 0x7A); };

        std::string res;
        res.reserve(str.size());

        size_t fast = 0, slow = 0;

        while (slow < str.size())
        {
            fast = str.find('\x1b', fast);
            if (fast == str.npos)fast = str.size();
            res.append(str.substr(slow, fast));
            for (; fast < str.size(); fast++)
                if (is_charactor(str[fast]))
                    break;
            slow = ++fast;
        }

        return res;
    }

    // 检查字符串内容是否全部合法，如不合法则报错
    inline bool check_content(
        const std::string& str,
        std::function<bool(char)> is_valid = [](char c) {return c > 0x20 && c < 0x7F; },
        bool throwExpt = true
    ) {
        for (const auto& c : str)
            if (!is_valid(c))
                if (throwExpt)
                    throw exceptions::common("Invalid character detected");
                else return false;
        return true;
    }

    // 用给定的字符切分字符串
    inline std::list<std::string> split_by(
        const std::string& str,
        const std::string& delimiter = " "
    ) {
        auto slow = str.cbegin();
        auto fast = str.cbegin();
        std::list<std::string> res;

        while (fast != str.cend())
        {
            fast = std::search(slow, str.cend(), delimiter.cbegin(), delimiter.cend());
            res.emplace_back(slow, fast);
            if (fast != str.cend())
                fast += delimiter.size();
            slow = fast;
        }
        return res;
    }

    inline std::list<std::string> split_by(
        const std::string& str,
        const std::list<std::string>& deliniters
    ) {
        std::list<std::string> res = { str };

        for (const auto& deliniter : deliniters)
        {
            std::list<std::string> strs;
            strs.swap(res);
            for (const auto& str : strs)
                res.splice(res.end(), split_by(str, deliniter));
        }

        return res;
    }

    template<typename C>
        requires requires(const C& c) {
            { c.begin() } -> std::input_or_output_iterator;
            { c.end() } -> std::sentinel_for<decltype(c.begin())>;
            { std::stringstream{} << (*(c.begin())) }->std::convertible_to<std::stringstream>;
            { c.empty() }->std::convertible_to<bool>;
    }
    std::string container_to_string(const C& c)
    {
        if (c.empty())return {};
        std::stringstream res;
        res << "[ ";
        for (const auto& i : c)
            res << i << ", ";
        res << " ]";
        return res.str();
    }
}