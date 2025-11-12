#pragma once
#include "pch.h"
#include "framework.h"

#include "concepts.h"
#include "labourer_exception.h"

namespace HYDRA15::Union::labourer
{
    // 无锁队列采用环形缓冲区 + 序号标记实现
    // 定长缓冲区
    // 可能存在忙等问题。要求元素有空构造
    // 性能测试：
    //                           debug       release
    //  std::queue + std::mutex 20w tps     100w tps
    //  lockless_queue<1M>     250w tps     300w tps
    template<typename T, size_t bufSize>
    class lockless_queue
    {
        struct cell { std::atomic<size_t> seqNo{}; T data{}; };
        static constexpr size_t retryTimesBeforeYield = 32;
    private:
        alignas(64) std::unique_ptr<cell[]> buffer = std::make_unique<cell[]>(bufSize);
        alignas(64) std::atomic<size_t> pNextEnque = 0;
        alignas(64) std::atomic<size_t> pNextDeque = 0;
        std::atomic_bool working = true;

    public:
        lockless_queue();
        lockless_queue(const lockless_queue&) = delete;
        lockless_queue(lockless_queue&&) = default;

        // 入出队
        template<typename U>
            requires framework::is_really_same_v<T,U>
        void push(U&&);
        T pop();
        template<typename U>
            requires framework::is_really_same_v<T, U>
        bool try_push(U&&);
        std::pair<bool, T> try_pop();

        // 信息和控制
        size_t size() const;
        size_t empty() const;
        void notify_exit();
    };


    template<typename T, size_t bufSize>
    inline lockless_queue<T, bufSize>::lockless_queue()
    {
        for (size_t i = 0; i < bufSize; i++)
            buffer[i].seqNo = i;
    }

    template<typename T, size_t bufSize>
    template<typename U>
        requires framework::is_really_same_v<T, U>
    inline void lockless_queue<T, bufSize>::push(U&& item)
    {
        while (working.load(std::memory_order_relaxed))
        {
            for (size_t i = 0; i < retryTimesBeforeYield; i++)
                if (try_push(std::forward<U>(item)))
                    return;
            std::this_thread::yield();
        }
    }

    template<typename T, size_t bufSize>
    inline T lockless_queue<T, bufSize>::pop()
    {
        while (working.load(std::memory_order_relaxed))
        {
            for (size_t i = 0; i < retryTimesBeforeYield; i++)
                if (auto [success, item] = try_pop(); success)
                    return item;
            std::this_thread::yield();
        }
        return T{};
    }

    template<typename T, size_t bufSize>
    template<typename U>
        requires framework::is_really_same_v<T, U>
    inline bool lockless_queue<T, bufSize>::try_push(U&& item)
    {
        size_t seq = pNextEnque.load(std::memory_order_relaxed);
        size_t p = seq % bufSize;
        if (buffer[p].seqNo.load(std::memory_order_acquire) != seq)
            return false;
        if (!pNextEnque.compare_exchange_strong(seq, seq + 1, std::memory_order_relaxed))
            return false;
        buffer[p].data = std::forward<U>(item);
        buffer[p].seqNo.store(seq + 1, std::memory_order_release);
        return true;
    }

    template<typename T, size_t bufSize>
    inline std::pair<bool, T> lockless_queue<T, bufSize>::try_pop()
    {
        size_t seq = pNextDeque.load(std::memory_order_relaxed);
        size_t p = seq % bufSize;
        if (buffer[p].seqNo.load(std::memory_order_acquire) != seq + 1)
            return { false,T{} };
        if (!pNextDeque.compare_exchange_strong(seq, seq + 1, std::memory_order_relaxed))
            return { false,T{} };
        T item = std::move(buffer[p].data);
        buffer[p].seqNo.store(seq + bufSize, std::memory_order_release);
        return { true,std::move(item) };
    }

    template<typename T, size_t bufSize>
    inline size_t lockless_queue<T, bufSize>::size() const
    {
        const size_t enq = pNextEnque.load(std::memory_order_acquire);
        const size_t deq = pNextDeque.load(std::memory_order_acquire);
        return (enq >= deq) ? (enq - deq) : std::numeric_limits<size_t>::max() - deq + enq;
    }

    template<typename T, size_t bufSize>
    inline size_t lockless_queue<T, bufSize>::empty() const
    {
        return size() == 0;
    }

    template<typename T, size_t bufSize>
    inline void lockless_queue<T, bufSize>::notify_exit()
    {
        working.store(false, std::memory_order_relaxed);
    }
}