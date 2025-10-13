#include "pch.h"
#include "utility.h"

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

    std::string hex_heap(const unsigned char* pBegin, unsigned int size, const std::string& title, unsigned int preLine)
    {
        std::string str = std::format("   -------- {} --------   \n", title);
        str.reserve((preLine * 0x4 + 0x20) * (size / preLine + 1) + 0x100 + title.size());

        // 打印表头
        str += "          ";
        for (unsigned int i = 0; i < preLine; i++)
            str += std::format("{:02X} ", i);
        str += "\n\n";

        // 打印数据
        std::string dataStr, charStr;
        dataStr.reserve(preLine * 3);
        charStr.reserve(preLine);
        for (unsigned int i = 0; i < size; i++)
        {
            if (i % preLine == 0)   // 行头地址
                str += std::format("{:08X}  ", i);

            dataStr += std::format("{:02X} ", pBegin[i]);
            if (pBegin[i] >= 0x20 && pBegin[i] <= 0x7E)
                charStr += pBegin[i];
            else
                charStr += ' ';

            if ((i + 1) % preLine == 0)
            {
                str += std::format("{}  |{}| \n", dataStr, charStr);
                dataStr.clear();
                charStr.clear();
            }

            if (i == size - 1 && (i + 1) % preLine != 0)  // 最后一行对齐
            {
                dataStr += std::string("   ") * (preLine - i % preLine - 1);
                charStr += std::string(" ") * (preLine - i % preLine - 1);
                str += std::format("{}  |{}| \n", dataStr, charStr);
                dataStr.clear();
                charStr.clear();
            }
        }

        return str;
    }

    std::unordered_map<std::string, std::string> parse_propreties(const std::string& ppts)
    {

        // 预处理：去除所有的空格，处理转义字符
        std::string content;
        content.reserve(ppts.size());
        for (size_t i = 0; i < ppts.size(); i++)
        {
            switch (ppts[i])
            {
                // 去除空格
            case ' ':
                break;
                // 处理转义
            case '\\':
                if (i + 1 >= ppts.size())
                    break;
                switch (ppts[i + 1])
                {
                    // 普通转义
                case 't': content += '\t'; i++; break;
                case 'n': content += '\n'; i++; break;
                case 'r': content += '\r'; i++; break;
                case 'f': content += '\f'; i++; break;
                case '\\': content += '\n'; i++; break;
                case ':': content += ':'; i++; break;
                    // 等号等会再处理
                case '=': content += ppts.substr(i, 2); i++; break;
                    // unicode 字符不处理
                case 'u':
                    if (i + 5 >= ppts.size())
                        throw exceptions::assistant::PropretiesParseFaild();
                    content += ppts.substr(i, 6);
                    i += 5;
                    break;
                    // 换行符直接去除
                case '\n': i++; break;
                    // 不知道的保留原样
                default: content += ppts.substr(i, 2); i++; break;
                }
                break;
                // 注释，一直删到行尾
            case '!':
            case '#':
                for (; i < ppts.size(); i++)
                    if (ppts[i] == '\n')
                        break;
                break;
                // 其余字符直接拷贝
            default:
                content += ppts[i];
            }
        }

        // 处理1：按行分割后按分隔符 = 分割
        std::list<std::list<std::string>> entryLst;
        {
            std::list<std::string> itemLst = split_by(content, "\n");
            content.~basic_string();    // 节省空间
            for (const auto& item : itemLst)
                entryLst.push_back(split_by(item, "="));
        }

        // 处理2：处理 \n 转义
        for (auto& entryPair : entryLst)
        {
            auto it = entryPair.begin();
            while (it != entryPair.end())
            {
                if (it->back() == '\\')
                {
                    auto current = it++;
                    current->pop_back();
                    *current += "=" + *it;
                    it = entryPair.erase(it);
                }
                else
                    it++;
            }
        }

        // 以上处理完成后，每个 list 中应该只有两个元素，分别为键和值
        std::unordered_map<std::string, std::string> res;
        for (auto& entryPair : entryLst)
        {
            if (entryPair.size() != 2)
                throw exceptions::assistant::PropretiesParseFaild();
            res[entryPair.front()] = entryPair.back();
        }

        return res;
    }
    
}