#pragma once
#pragma once
#include "framework.h"
#include "pch.h"

#include "concepts.h"
#include "expressman_exception.h"
#include "expressman_interfaces.h"

namespace HYDRA15::Union::expressman
{
    // 用于从数据包构造对象的工厂
    // 应当在使用可打包对象之前向工厂注册构造函数
    class factory
    {
    public:
        using constructor = std::function<packable::objects(packable::datablocks)>;
        using constructor_tab = std::unordered_map<std::string, constructor>;

    private:
        constructor_tab ct;
        std::shared_mutex smt;

    public:
        packable::objects build(const std::list<archive>& archlst);    // 构造对象，给定的列表中只能由一个类，并且需要有完整的数据

        void regist(std::string name, const std::function<packable::objects(packable::datablocks)>& constructor);  // 注册构造函数
        bool unregist(std::string name);    // 移除构造函数
        bool contains(std::string name);    // 检查构造函数
    };
}