#pragma once
#include "framework.h"
#include "pch.h"

#include "archivist_types.h"

namespace HYDRA15::Union::archivist
{
    /******************************* 数据层接口 *******************************/
    // 字段信息
    class field_spec
    {
    public:
        virtual ~field_spec() = default;

        // 获取字段数据
    public:
        virtual types::ID ID() = 0;
        virtual std::string name() = 0;
        virtual types::type type() = 0;
        virtual std::array<types::BYTE, 3> marks() = 0;
    };


    // 数据行的包装器类，应当可以通过此对象修改表中的原始数据
    class entry
    {
    public:
        virtual ~entry() = default;

        // 获取、写入记录项
    public:
        virtual types::field at(field_spec) = 0;
        virtual entry set(field_spec, const types::field&) = 0;

        // 信息接口
    public:
        virtual types::ID ID() = 0;
        virtual types::ID last_ver() = 0;
        virtual std::array<types::BYTE, 8> marks() = 0;

        // 将数据行对象直接作为迭代器使用，需要支持迭代器的操作
    public:
        virtual entry& operator++() = 0;
        virtual entry& operator--() = 0;
        virtual bool operator==(const entry&) = 0;
        virtual bool operator!=(const entry&) = 0;

        // 行锁
    public:
        virtual void lock() = 0;
        virtual void unlock() = 0;
        virtual bool try_lock() = 0;
    };

    // 数据表
    class tablet
    {
    public:
        virtual ~tablet() = default;

        // 增删改查接口
    public:
        // 查询返回表记录的引用，应当可以通过引用修改表项
        virtual entry at(types::ID) = 0;                                                               // 通过 ID 查询
        virtual std::list<entry> at(std::function<bool(const entry&)>) = 0;     // 通过过滤器查找

        // 信息和控制接口
    public:
        virtual types::ID load(types::ID begin, types::ID end) = 0; // 从磁盘加载指定范围的数据
        virtual types::ID sync(types::ID begin, types::ID end) = 0; // 将指定范围的数据写入磁盘，但是不改变内存中的数据
        virtual types::ID flush(types::ID begin, types::ID end) = 0;// 将指定范围的数据写入磁盘，并删除内存中对应的数据

        virtual types::ID size() = 0;   // 返回表记录条数
        virtual size_t memsize() = 0;   // 返回表所占内存大小

        // 迭代器接口
    public:
        virtual entry begin() = 0;
        virtual entry end() = 0;

        // 表锁
    public:
        virtual void lock() = 0;
        virtual void unlock() = 0;
        virtual bool try_lock() = 0;
    };


}