#pragma once
#include "pch.h"
#include "framework.h"

#include "string_utilities.h"
#include "concepts.h"

namespace HYDRA15::Union::assistant
{
    // 输出十六进制的原始数据和对应的ascii字符
    inline std::string hex_heap(const unsigned char* pBegin, unsigned int size, const std::string& title = "Hex Heap", unsigned int preLine = 32)
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

    // 将字节转换为十六进制（高效版）
    inline std::string byte_to_hex(const unsigned char* pBegin, unsigned int size)
    {
        static char lut[16] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' };

        std::string res;
        res.reserve(2LL * size);
        for (const unsigned char* p = pBegin; p < pBegin + size; p++)
        {
            res.push_back(lut[(*p) & 0xF]);
            res.push_back(lut[((*p) >> 4)]);
        }

        return res;
    }
    

    // 解析 propreties 文件
    // 强制要求键值分隔符为 = ，unicode字符保持原样，
    inline std::unordered_map<std::string, std::string> parse_propreties(const std::string& ppts)
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
                        throw exceptions::common::UnsupportedFormat("Unicode charactor format must be '\\uxxxx'");
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
                throw exceptions::common::UnsupportedFormat("Key - value pair joined by '='");
            res.emplace(std::pair{ std::move(entryPair.front()),std::move(entryPair.back()) });
        }

        return res;
    }

    // 解析 csv 文件
    // 支持引号不分割，但是不删除引号
    inline std::list<std::list<std::string>> parse_csv(const std::string& csv)
    {
        std::list<std::list<std::string>> res;

        std::list<std::string> lines = split_by(csv, "\n");
        for (const auto& line : lines)
        {
            std::list<std::string> entries = split_by(line, ",");
            auto ientry = entries.begin();
            while (ientry != entries.end()) // 处理引号不分割
            {
                if (strip(*ientry).front() = '"')
                {
                    auto next = ientry;
                    next++;
                    while (next != entries.end() && strip(*next).back() != '"')
                    {
                        (*ientry).append(*next);
                        next = entries.erase(next);
                    }
                    if (next != entries.end())
                        (*ientry).append(*next);
                }
                ientry++;
            }
            res.push_back(entries);
        }

        return res;
    }


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
    inline size_t multiple_m_not_less_than_n(size_t m, size_t n)
    {
        if (m == 0) return 0;
        return ((n + m - 1) / m) * m;
    }

    // 计算不小于某数的2的幂次
    inline size_t power_of_2_not_less_than_n(size_t n)
    {
        if (n == 0) return 0;
        size_t power = 1;
        while (power < n)
            power <<= 1;
        return power;
    }


    // 集合操作
    namespace set_operation
    {
        template<typename T, template<typename ...>typename S>
        concept is_set_container =
            framework::is_really_same_v<S<T>, std::set<T>> ||
            framework::is_really_same_v<S<T>, std::unordered_set<T>>;

        // 并集
        template<typename T, template<typename ...>typename S>
            requires is_set_container<T, S>
        inline S<T> operator+(const S<T>& l, const S<T>& r)
        {
            S<T> res;
            res.insert_range(l);
            res.insert_range(r);
            return res;
        }

        // 交集
        template<typename T, template<typename ...>typename S>
            requires is_set_container<T, S>
        inline S<T> operator&(const S<T>& l, const S<T>& r)
        {
            if (l.size() < r.size())
            {
                S<T> res;
                for (const auto& i : l)
                    if (r.contains(i))
                        res.insert(i);
                return res;
            }
            else 
            {
                S<T> res;
                for (const auto& i : r)
                    if (l.contains(i))
                        res.insert(i);
                return res;
            }
        }

        // 查集
        template<typename T, template<typename ...>typename S>
            requires is_set_container<T, S>
        inline S<T> operator-(const S<T>& l, const S<T>& r)
        {
            S<T> res;
            for (const auto& i : l)
                if (!r.contains(i))
                    res.insert(i);
            return res;
        }

        // 对称交集
        template<typename T, template<typename ...>typename S>
            requires is_set_container<T, S>
        inline S<T> operator|(const S<T>& l, const S<T>& r)
        {
            return (l + r) - (l & r);
        }
    }
    
   

}