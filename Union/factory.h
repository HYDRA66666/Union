#pragma once
#pragma once
#include "framework.h"
#include "pch.h"

#include "concepts.h"
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
        packable::objects build(const std::list<packet>& archlst)    // 构造对象，给定的列表中只能由一个类，并且需要有完整的数据
        {
            if (archlst.empty())    // 空列表
                return packable::objects();

            std::string className = extract_name(archlst.front());
            std::shared_lock slk(smt);

            // 限定列表中只能有一个类
            for (const auto& a : archlst)
                if (extract_name(a) != className)
                    throw exceptions::common{ framework::libID.expressman, 0xB01, "The provided data is incomplete" };
            
            constructor cstr;

            {
                std::shared_lock slk(smt);
                // 检查构造函数是否存在
                if (!ct.contains(className))
                    throw exceptions::common{ framework::libID.expressman, 0xB02, "The provided data contains an unknown class" };
                cstr = ct.at(className);
            }

            return unpack(archlst, cstr);
        }

        void regist(std::string name, const std::function<packable::objects(packable::datablocks)>& cstr)  // 注册构造函数
        {
            std::unique_lock ulk(smt);
            ct.emplace(std::pair<std::string, constructor>{ name, cstr });
        }

        bool unregist(std::string name)    // 移除构造函数
        {
            std::unique_lock ulk(smt);
            return ct.erase(name);
        }

        bool contains(std::string name)    // 检查构造函数
        {
            std::shared_lock ulk(smt);
            return ct.contains(name);
        }
    };
}