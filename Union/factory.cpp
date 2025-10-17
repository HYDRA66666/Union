#include "factory.h"
#include "pch.h"

namespace HYDRA15::Union::expressman
{
    packable::objects factory::build(const std::list<archive>& archlst)
    {
        if (archlst.empty())    // 空列表
            return packable::objects();

        std::string className = extract_name(archlst.front());
        std::shared_lock slk(smt);

        // 限定列表中只能有一个类
        for (const auto& a : archlst)
            if (extract_name(a) != className)
                throw exceptions::expressman::FactoryContaminatedData();
        // 检查构造函数是否存在
        if (!ct.contains(className))
            throw exceptions::expressman::FactoryUnknownClass();

        return unpack(archlst, ct.at(className));
    }

    void factory::regist(std::string name, const constructor& cstr)
    {
        std::unique_lock ulk(smt);
        if (ct.contains(name))
            throw exceptions::expressman::FactoryUnknownClass();
        ct.emplace(std::pair<std::string, constructor>{ name, cstr });
    }

    bool factory::unregist(std::string name)
    {
        std::unique_lock ulk(smt);
        return ct.erase(name);
    }

    bool factory::contains(std::string name)
    {
        std::unique_lock ulk(smt);
        return ct.contains(name);
    }
}
