#pragma once
#include "framework.h"
#include "pch.h"

#include "concepts.h"
#include "expressman_interfaces.h"
#include "background.h"

namespace HYDRA15::Union::expressman
{
    // 用于根据地址投递到下一站
    // 支持单播与组播。单播地址和组播地址的类型相同，但是不应当重合
    template<framework::hash_key A>
    class basic_mailrouter :virtual public collector<A>
    {
    public:
        using router_map = std::unordered_map<A, std::weak_ptr<collector<A>>>;
        using group_router_map = std::unordered_map<A, std::function<std::generator<A>()>>;
        

    protected:
        router_map rmap; // 路由表，地址 -> 下一站
        group_router_map grmap;  // 组播路由表，组地址 -> 地址列表
        std::shared_mutex smt;

    public:
        basic_mailrouter() = default;
        basic_mailrouter(router_map rm, group_router_map grm = group_router_map()) :rmap(rm), grmap(grm) {  }
        basic_mailrouter(const basic_mailrouter&) = delete;
        basic_mailrouter(basic_mailrouter&&) = delete;
        virtual ~basic_mailrouter() = default;

        virtual unsigned int post(const std::shared_ptr<const postable<A>>& pkg) override
        {
            std::shared_lock slk(smt);
            pkg->next_route();
            unsigned int posted = 0;
            A dest = pkg->destination();

            // 匹配组播路由
            const auto& grit = grmap.find(dest);    
            if (grit != grmap.end())
                for (const auto& i : (grit->second)())
                {
                    const auto& rit = rmap.find(i);
                    if (rit != rmap.end() && !rit->second.expired())
                    {
                        rit->second.lock()->post(std::dynamic_pointer_cast<const postable<A>>(pkg->clone()));  // 组播路由创建新的对象
                        posted++;
                    }
                }

            // 匹配单播路由
            const auto& rit = rmap.find(dest);
            if (rit != rmap.end() && !rit->second.expired())
            {
                rit->second.lock()->post(pkg);
                posted++;
            }
            return posted;
        }

        // 修改路由表接口
        bool add(A addr, const std::weak_ptr<collector<A>>& pc)  // 添加路由表项
        {
            std::unique_lock ul(smt);
            if (rmap.contains(addr))
                return false;
            rmap[addr] = pc;
            return true;
        }
        bool add(A addr, std::function<std::generator<A>()> gener) // 添加组播路由表项
        {
            std::unique_lock ul(smt);
            if (grmap.contains(addr))
                return false;
            grmap[addr] = gener;
            return true;
        }
        bool add(A addr, const std::list<A>& lst)   // 通过列表添加组播路由表项
        {
            std::unique_lock ul(smt);
            if (grmap.contains(addr))
                return false;
            grmap[addr] = [lst = lst]() -> std::generator<A> {for (const auto& i : lst)co_yield i; };
            return true;
        }
        void cleanup()  // 清理失效的路由表项
        {
            std::unique_lock ul(smt);
            auto it = rmap.begin();
            while (it != rmap.end())
                if (it->second.expired())
                    it = rmap.erase(it);
                else
                    it++;
        }

        // 高级管理接口
        router_map& fetch_rmap() { return rmap; }   // 直接获取整个路由表
        group_router_map& fetch_grmap() { return grmap; }   // 直接获取整个路由表
        auto lock() { return smt.lock(); }
        auto unlock() { return smt.unlock(); }
        auto try_lock() { return smt.try_lock(); }
    };
}