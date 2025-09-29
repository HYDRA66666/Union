#include "factory.h"

namespace HYDRA15::Union::archivist
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
                throw exceptions::archivist::FactoryContaminatedData();
        // 检查构造函数是否存在
        if (!ct.contains(className) || !ct.fecth(className))
            throw exceptions::archivist::FactoryUnknownClass();

        return unpack(archlst, ct.fecth(className));
    }

    void factory::regist(std::string name, const std::function<packable::objects(packable::datablocks)>& constructor)
    {
        std::unique_lock ulk(smt);
        return ct.regist(name, constructor);
    }

    bool factory::unregist(std::string name)
    {
        std::unique_lock ulk(smt);
        return ct.unregist(name);
    }

    bool factory::contains(std::string name)
    {
        std::unique_lock ulk(smt);
        return ct.contains(name);
    }
}
