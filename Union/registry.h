#pragma once
#include "framework.h"
#include "pch.h"

#include "concepts.h"
#include "archivist_exception.h"

namespace HYDRA15::Union::archivist
{
    // 处理注册相关逻辑
    // 模板参数 K : 键类型，要求满足哈希键约束
    // 模板参数 V : 值类型，不做要求
    // 不负责线程安全，线程安全内容请自行处理
    // 如果不需要上锁，可以使用 NotALock 作为锁类型，其满足锁约束，但不会进行任何实际操作
    /***************************** 基 类 *****************************/
    // 基础注册机模板，支持任意类型的键和值
    template<framework::hash_key K, typename V>
    class basic_registry
    {
        // 类型定义
    public:
        using regtab = std::unordered_map<K, V>;


        // 核心数据
    protected:
        regtab tab;
        size_t max = 0;

        // 构造与析构
    public:
        basic_registry(size_t maxSize)
            :max(maxSize)
        {
            if (max > tab.max_size())
                throw exceptions::archivist::RegistryTabletInvalidMaxSize();
        }
        basic_registry() : max(0) {}

        // 注册
        void regist(const K& key, const V& value)
        {
            if (max > 0 && tab.size() >= max)
                throw exceptions::archivist::RegistryTabletFull();
            if (tab.size() >= tab.max_size())
                throw exceptions::archivist::RegistryTabletFull();
            if (tab.contains(key))
                throw exceptions::archivist::RegistryKeyExists();

            tab[key] = value;
        }
        void regist(const K& key, V&& value)
        {
            if (max > 0 && tab.size() >= max)
                throw exceptions::archivist::RegistryTabletFull();
            if(tab.size() >= tab.max_size())
                throw exceptions::archivist::RegistryTabletFull();
            if (tab.contains(key))
                throw exceptions::archivist::RegistryKeyExists();

            tab[key] = std::forward<V>(value);
        }
        void unregist(const K& key)
        {
            tab.erase(key);
        }
        V& fecth(const K& key)
        {
            if (!tab.contains(key))
                throw exceptions::archivist::RegistryKeyNotFound();
            return tab[key];
        }

        // 查询
        bool contains(const K& key) const
        {
            return tab.contains(key);
        }
        size_t size() const
        {
            return tab.size();
        }

        // 迭代器
        using iterator = typename regtab::iterator;
        iterator begin() { return tab.begin(); }
        iterator end() { return tab.end(); }
    };


    /***************************** 特化类 *****************************/
    // 整数键注册机，支持被动注册和懒注册
    template<typename V>
    class int_registry : public basic_registry<unsigned long long, V>
    {
        // 类型定义
    public:
        using uint_index = unsigned long long;
        // 核心数据
        uint_index current = 0;
        uint_index start = 0;
        using basic_registry<uint_index, V>::tab;
        using basic_registry<uint_index, V>::max;

        // 构造与析构
    public:
        int_registry(uint_index startKey = 0, size_t maxSize = 0)
            : basic_registry<uint_index, V>(maxSize), start(startKey), current(startKey)
        {
        }

        // 辅助函数
    private:
        void find_next_key()
        {
            if (current != std::numeric_limits<uint_index>::max())
                current++;   // 默认currentKey已使用
            while (current != std::numeric_limits<uint_index>::max() && tab.contains(current))
                current++;
            if (current == std::numeric_limits<uint_index>::max() && tab.contains(current)) // 若达到最大值，则重新扫描整整表，查找是否有空缺位置
            {
                current = start;
                while (current != std::numeric_limits<uint_index>::max() && tab.contains(current))
                    current++;
                if (current == std::numeric_limits<uint_index>::max()) // 找不到空缺位置，则抛出异常
                    throw exceptions::archivist::RegistryTabletFull();
            }
        }

        // 注册
    public:
        // 被动注册：传入值，注册机分配键
        uint_index regist(V&& value)
        {
            if (max > 0 && tab.size() >= max)
                throw exceptions::archivist::RegistryTabletFull();
            if (tab.size() >= tab.max_size())
                throw exceptions::archivist::RegistryTabletFull();
            find_next_key();

            tab[current] = std::forward<V>(value);
            return current;
        }

        // 懒注册：不传入值，注册机分配键和默认值
        uint_index regist()
        {
            if (max > 0 && tab.size() >= max)
                throw exceptions::archivist::RegistryTabletFull();
            if (tab.size() >= tab.max_size())
                throw exceptions::archivist::RegistryTabletFull();
            find_next_key();

            tab[current];
            return current;
        }
    };
}