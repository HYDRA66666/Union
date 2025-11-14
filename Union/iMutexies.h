#pragma once
#include "pch.h"
#include "framework.h"

namespace HYDRA15::Union::labourer
{
    // 使用原子变量实现的读写锁，比 std::mutex 有更好的性能
    // 性能测试：
    //                           debug       release
    //  无锁（纯 std::atomic）4450w tps     6360w tps
    //  std::shared_mutex      480w tps      670w tps
    //  atomic_shared_mutex    920w tps     1470w tps
    template<size_t retreatFreq = 32>
    class atomic_shared_mutex_temp
    {
    private:
        std::atomic_bool writer = false;
        std::atomic<size_t> readers = 0;

    public:
        void lock()
        {
            size_t i = 0;
            while (true)
            {
                bool expected = false;
                if (writer.compare_exchange_weak(
                    expected, true,
                    std::memory_order_acq_rel, std::memory_order_relaxed
                ))break;
                i++;
                if (i >= retreatFreq)std::this_thread::yield();
            }
            while (true)
            {
                if (readers.load(std::memory_order_acquire) == 0)break;
                i++;
                if (i > retreatFreq)std::this_thread::yield();
            }
        }
        void unlock() { writer.store(false, std::memory_order_release); }
        bool try_lock()
        {
            bool expected = false;
            if (!writer.compare_exchange_weak(
                expected, true,
                std::memory_order_release, std::memory_order_relaxed
            ))return false;
            if (!readers.load(std::memory_order_acquire) == 0) 
            { 
                writer.store(false, std::memory_order_release); 
                return false; 
            }
            return true;
        }
        
        void lock_shared()
        {
            size_t i = 0;
            while (true)
            {
                if (!writer.load(std::memory_order_acquire))
                {
                    readers.fetch_add(1, std::memory_order_acquire);
                    if (!writer.load(std::memory_order_release))return;
                    readers.fetch_add(-1, std::memory_order_release);
                }
                i++;
                if (i > retreatFreq)std::this_thread::yield();
            }
        }
        void unlock_shared() { readers.fetch_add(-1, std::memory_order_release); }
        bool try_lock_shared()
        {
            if (!writer.load(std::memory_order_acquire))
            {
                readers.fetch_add(1, std::memory_order_acquire);
                if (!writer.load(std::memory_order_release))return true;
                readers.fetch_add(-1, std::memory_order_release);
            }
            return false;
        }
    };

    using atomic_shared_mutex = atomic_shared_mutex_temp<>;

}