#pragma once
#include "framework.h"
#include "pch.h"

#include "concepts.h"
#include "expressman_interfaces.h"

namespace HYDRA15::Union::expressman
{
    // 用于收取并存储 package 的容器
    template<framework::hash_key A>
    class basic_mailbox :virtual public collector<A>
    {
    protected:
        std::list<std::shared_ptr<const postable<A>>> lst;
        std::mutex lk;
        std::condition_variable cv;
        
    public:
        basic_mailbox() = default;
        basic_mailbox(const basic_mailbox&) = delete;
        basic_mailbox(basic_mailbox&&) = delete;
        virtual ~basic_mailbox() = default;

        virtual bool post(const std::shared_ptr<const postable<A>>& pkg) override
        {
            std::unique_lock ul(lk);
            lst.push_back(pkg);
            return true;
        }

        std::shared_ptr<const postable<A>> fetch()
        {
            std::unique_lock ul(lk);
            while (lst.empty())
                cv.wait(ul);
            std::shared_ptr<const postable<A>> ptr = lst.front();
            lst.pop_front();
            return ptr;
        }
        std::list<std::shared_ptr<const postable<A>>> fetch_all()
        {
            std::unique_lock ul(lk);
            while (lst.empty())
                cv.wait(ul);
            std::list<std::shared_ptr<const postable<A>>> ppkgs;
            lst.swap(ppkgs);
            return ppkgs;
        }

        size_t size() { return lst.size(); }
        bool empty() { return lst.empty(); }
        void clear()
        {
            std::unique_lock ul(lk);
            return lst.clear();
        }
    };
}