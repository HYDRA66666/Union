#pragma once
#include "framework.h"
#include "pch.h"

#include "archivist_types.h"

namespace HYDRA15::Union::archivist
{
    /******************************* 数据层接口 *******************************/
    // 数据层：组织存储数据、管理磁盘io、提供表访问接口
    // 字段信息
    struct field_spec
    {
        types::ID id;   // 仅用于内部加速
        std::string name;
        types::type type;
    };


    // 数据行
    class entry
    {
    public:
        virtual ~entry() = default;

        // 获取、写入记录项
        virtual types::field at(const field_spec&) = 0;
        virtual entry& set(const field_spec&, const types::field&) = 0;

        // 信息接口
    public:
        virtual types::ID ID() = 0;

        // 将数据行对象直接作为迭代器使用，需要支持迭代器的操作
    public:
        virtual entry& operator++() = 0;
        virtual std::strong_ordering operator<=>(const entry&) = 0;

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

        // 获取字段信息接口
    public:
        virtual field_spec get_field_spec(const std::string&) = 0;  // 通过字段名获取

        // 增删改查接口
    public:
        // 查询返回表记录的引用，应当可以通过引用修改表项
        virtual entry& create() = 0;        // 创建新表项，返回新建的表项，从返回的对象向其中写入数据
        virtual void drop(types::ID) = 0;   // 删除表项
        virtual std::shared_ptr<entry> at(types::ID) = 0;                                   // 通过 ID 查询
        virtual std::list<std::shared_ptr<entry>> at(std::function<bool(const entry&)>) = 0;// 通过过滤器查找

        // 信息和控制接口
    public:
        virtual types::ID size() = 0;   // 返回表记录条数
        virtual size_t memsize() = 0;   // 返回表所占内存大小

        // 迭代器接口
    public:
        virtual std::shared_ptr<entry> begin() = 0;
        virtual std::shared_ptr<entry> end() = 0;

        // 表锁
    public:
        virtual void lock() = 0;
        virtual void unlock() = 0;
        virtual bool try_lock() = 0;
    };


}