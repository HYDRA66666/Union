#pragma once
#include "framework.h"
#include "pch.h"

#include "concepts.h"
#include "archivist_interfaces.h"
#include "expressman_exception.h"

namespace HYDRA15::Union::expressman
{
    // 可被投递的接口
    // 模板参数为地址类型
    template<framework::hash_key A>
    class postable : virtual public archivist::notype
    {
    public:
        virtual A origin() const { return std::string(); }  // 指示源地址
        virtual A destination() const = 0;   // 指示目标地址

        // 可选：指定路由路径时使用
        // 使得指示目的地的指针在路由路径列表中向后移动一位，如果成功返回true，
        // 否则，如不存在路由路径或者已经达到最终目的地，返回false
        virtual bool next_route() const { return false; }

        virtual ~postable() = default;
    };

    // 基本包裹可投递、可发送
    template<framework::hash_key A>
    class basic_package : virtual public postable<A>, virtual public archivist::packable
    { };

    using package = basic_package<std::string>;


    // 实现此接口的类可以接收 postable 或其子类
    // 用于在多级路由中抹除层级差异
    // 模板参数为地址类型
    template<framework::hash_key A>
    class collector
    {
    public:
        virtual unsigned int post(const std::shared_ptr<const postable<A>>& pkg) = 0;    

        virtual ~collector() = default;
    };

    
}