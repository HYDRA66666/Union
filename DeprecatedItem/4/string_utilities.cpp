#include "pch.h"
#include "string_utilities.h"

namespace HYDRA15::Union::assistant
{
    std::string operator*(std::string str, size_t count) {
        std::string result;
        result.reserve(str.size() * count);
        for (size_t i = 0; i < count; ++i) {
            result += str;
        }
        return result;
    }

    std::string strip_front(const std::string& str, std::function<bool(char)> is_valid)
    {
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

    std::string strip_back(const std::string& str, std::function<bool(char)> is_valid)
    {
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

    std::string strip(const std::string& str, std::function<bool(char)> is_valid)
    {
        return strip_back(strip_front(str, is_valid), is_valid);
    }

    std::string strip_all(const std::string& str, std::function<bool(char)> is_valid)
    {
        std::string res;
        res.reserve(str.size());

        for (char c : str)
            if (is_valid(c))
                res.push_back(c);

        res.shrink_to_fit();
        return res;
    }

    std::string strip_color(const std::string& str)
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

    std::string strip_ansi_secquence(const std::string& str)
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

    void check_content(const std::string& str, std::function<bool(char)> is_valid)
    {
        for (const auto& c : str)
            if (!is_valid(c))
                throw exceptions::assistant::UtilityInvalidChar();
    }

    std::list<std::string> split_by(const std::string& str, const std::string& delimiter)
    {
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

    std::list<std::string> split_by(const std::string& str, const std::list<std::string>& deliniters)
    {
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
}