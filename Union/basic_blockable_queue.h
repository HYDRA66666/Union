#pragma once
#include "pch.h"
#include "framework.h"

#include "concepts.h"

namespace HYDRA15::Union::labourer
{
    // 基于 std::queue std::mutex std::conditional_variable 的基本阻塞队列
    template<typename T>
    class basic_blockable_queue
    {
        std::queue<T> queue;
        std::mutex mtx;
        std::condition_variable cv;
        std::atomic_bool working = true;

    public:
        template<typename U>
            requires framework::is_really_same_v<T,U>
        void push(U&& item)
        {
            if (!working.load(std::memory_order_relaxed))return;
            std::unique_lock ul{ mtx };
            queue.push(std::forward<U>(item));
            cv.notify_one();
            return;
        }

        T pop()
        {
            std::unique_lock ul{ mtx };
            while (working.load(std::memory_order_relaxed) && queue.empty())cv.wait(ul);
            if (!working.load(std::memory_order_relaxed))return T{};
            T t = std::move(queue.front());
            queue.pop();
            return t;
        }

        size_t size() const { return queue.size(); }
        bool empty() const { return queue.empty(); }

        void notify_exit()
        {
            working.store(false, std::memory_order_relaxed);
            cv.notify_all();
        }
        
    };
}