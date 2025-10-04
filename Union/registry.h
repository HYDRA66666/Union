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
    template<framework::hash_key K, typename V, framework::map_container M = std::unordered_map<K,V>>
    class basic_registry
    {
        // 类型定义
    public:
        using regtab = M;


        // 核心数据
    protected:
        regtab tab;

        // 构造与析构
    public:
        basic_registry() = default;

        // 注册
        template<typename Val>
            requires framework::is_really_same_v<Val,V>
        void regist(const K& key, Val&& value)
        {
            if(!tab.try_emplace(key, std::forward<Val>(value)).second)
                throw exceptions::archivist::RegistryKeyExists();
            return;
        }
        auto unregist(const K& key)
        {
            return tab.erase(key);
        }
        V& fecth(const K& key)
        {
            try{return tab.at(key);}
            catch (const std::out_of_range&) { throw exceptions::archivist::RegistryKeyNotFound(); }
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
    template<typename V, framework::map_container M = std::unordered_map<unsigned long long, V>>
    class int_registry : public basic_registry<unsigned long long, V, M>
    {
        // 类型定义
    public:
        using uint_index = unsigned long long;
        // 核心数据
        std::atomic<uint_index> current = 0;
        const uint_index start = 0;
        using basic_registry<uint_index, V>::tab;

        // 构造与析构
    public:
        int_registry(uint_index startKey = 0)
            : basic_registry<uint_index, V, M>(), start(startKey), current(startKey)
        {
        }

        // 辅助函数
    private:
        uint_index find_next_key()
        {
            while(true)
            {
                uint_index c = current, old = c;
                if (c != std::numeric_limits<uint_index>::max())
                    c++;   // 默认currentKey已使用
                while (c != std::numeric_limits<uint_index>::max() && tab.contains(c))
                    c++;
                if (c == std::numeric_limits<uint_index>::max() && tab.contains(c)) // 若达到最大值，则重新扫描整整表，查找是否有空缺位置
                    c = start;
                while (c != std::numeric_limits<uint_index>::max() && tab.contains(c))
                    c++;
                if (c == std::numeric_limits<uint_index>::max()) // 找不到空缺位置，则抛出异常
                    throw exceptions::archivist::RegistryTabletFull();
                if (current.compare_exchange_strong(old, c))
                    return current;
            }
        }

        // 注册
    public:
        // 被动注册：传入值，注册机分配键
        template<typename Val>
            requires framework::is_really_same_v<Val, V>
        uint_index regist(Val&& value)
        {
            using pair = decltype(tab.try_emplace(0, std::forward<Val>(value)));
            pair p;
            while ((p = tab.try_emplace(find_next_key(), std::forward<Val>(value))).second)
                return p.first->first;
            throw exceptions::archivist::RegistryTabletFull();
        }

        // 懒注册：不传入值，注册机分配键和默认值
        uint_index regist()
        {
            using pair = decltype(tab.try_emplace(0, V()));
            pair p;
            while ((p = tab.try_emplace(find_next_key(), V())).second)
                return p.first->first;
        }
    };
}