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
            requires framework::is_really_same_v<T, U>
        void push(U&& item)
        {
            if (!working.load(std::memory_order_acquire))return;
            std::unique_lock ul{ mtx };
            queue.push(std::forward<U>(item));
            cv.notify_one();
            return;
        }

        T pop()
        {
            std::unique_lock ul{ mtx };
            while (working.load(std::memory_order_acquire) && queue.empty())cv.wait(ul);
            if (!working.load(std::memory_order_acquire))return T{};
            if constexpr (std::is_move_constructible_v<T>)
            {
                T t = std::move(queue.front());
                queue.pop();
                return std::move(t);
            }
            else
            {
                T t = queue.front();
                queue.pop();
                return t;
            }
        }

        size_t size() const { return queue.size(); }
        bool empty() const { return queue.empty(); }

        void notify_exit()
        {
            working.store(false, std::memory_order_release);
            cv.notify_all();
        }

    };



    // 无锁队列采用环形缓冲区 + 序号标记实现
    // 定长缓冲区
    // 可能存在忙等问题。要求元素有空构造
    // 性能测试：
    //                           debug       release
    //  std::queue + std::mutex 20w tps     100w tps
    //  lockless_queue<1M>     250w tps     300w tps
    template<typename T, size_t bufSize, size_t maxRetreatFreq = 32>
    class lockless_queue
    {
        struct cell { std::atomic<size_t> seqNo{}; T data{}; };
    private:
        alignas(64) std::unique_ptr<cell[]> buffer = std::make_unique<cell[]>(bufSize);
        alignas(64) std::atomic<size_t> pNextEnque = 0;
        alignas(64) std::atomic<size_t> pNextDeque = 0;
        std::atomic_bool working = true;

    public:
        lockless_queue() { for (size_t i = 0; i < bufSize; i++)buffer[i].seqNo = i; }
        lockless_queue(const lockless_queue&) = delete;
        lockless_queue(lockless_queue&&) = default;

        // 入出队
        template<typename U>
            requires framework::is_really_same_v<T, U>
        void push(U&& item)
        {
            size_t retreatFreq = maxRetreatFreq;
            while (working.load(std::memory_order_acquire))
            {
                for (size_t i = 0; i < retreatFreq; i++)
                    if (try_push(std::forward<U>(item)))
                        return;
                if (retreatFreq > 1)retreatFreq /= 2;
                std::this_thread::yield();
            }
        }

        T pop()
        {
            size_t retreatFreq = maxRetreatFreq;
            while (working.load(std::memory_order_acquire))
            {
                for (size_t i = 0; i < retreatFreq; i++)
                    if (auto [success, item] = try_pop(); success)
                        if constexpr (std::is_move_constructible_v<T>) return std::move(item);
                        else return item;
                if (retreatFreq > 1)retreatFreq /= 2;
                std::this_thread::yield();
            }
            return T{};
        }

        template<typename U>
            requires framework::is_really_same_v<T, U>
        bool try_push(U&& item)
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

        std::pair<bool, T> try_pop()
        {
            size_t seq = pNextDeque.load(std::memory_order_relaxed);
            size_t p = seq % bufSize;
            if (buffer[p].seqNo.load(std::memory_order_acquire) != seq + 1)
                return { false,T{} };
            if (!pNextDeque.compare_exchange_strong(seq, seq + 1, std::memory_order_relaxed))
                return { false,T{} };
            if constexpr (std::is_move_constructible_v<T>)
            {
                T item = std::move(buffer[p].data);
                buffer[p].seqNo.store(seq + bufSize, std::memory_order_release);
                return { true, std::move(item) };
            }
            else
            {
                T item = buffer[p].data;
                buffer[p].seqNo.store(seq + bufSize, std::memory_order_release);
                return { true,item };
            }

        }

        // 信息和控制
        size_t size() const
        {
            const size_t enq = pNextEnque.load(std::memory_order_acquire);
            const size_t deq = pNextDeque.load(std::memory_order_acquire);
            return (enq >= deq) ? (enq - deq) : std::numeric_limits<size_t>::max() - deq + enq;
        }

        size_t empty() const 
        {  
            return pNextDeque.load(std::memory_order_acquire) ==
                pNextEnque.load(std::memory_order_acquire);
        }

        void notify_exit() { working.store(false, std::memory_order_release); }

    };
}