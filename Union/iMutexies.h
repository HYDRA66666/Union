#pragma once
#include "pch.h"
#include "framework.h"

namespace HYDRA15::Union::labourer
{
    template<size_t retreatFreq = 32>
    class atomic_mutex_temp
    {
    private:
        std::atomic_bool lck = false;

    public:
        void lock()
        {
            while (true)
            {
                for (size_t i = 0; i < retreatFreq; i++)
                    if (try_lock())return;
                std::this_thread::yield();
            }
        }

        void unlock() { lck.store(false, std::memory_order::release); }

        bool try_lock()
        {
            bool expected = false;
            return lck.compare_exchange_strong(expected, true, std::memory_order::acquire);
        }
    };

    using atomic_mutex = atomic_mutex_temp<>;

    // 使用原子变量实现的读写锁，比 std::shared_mutex 有更好的性能
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
                    std::memory_order::acquire, std::memory_order::relaxed
                ))break;
                i++;
                if (i >= retreatFreq)std::this_thread::yield();
            }
            while (true)
            {
                if (readers.load(std::memory_order::acquire) == 0)break;
                i++;
                if (i > retreatFreq)std::this_thread::yield();
            }
        }
        void unlock() { writer.store(false, std::memory_order::release); }
        bool try_lock()
        {
            bool expected = false;
            if (!writer.compare_exchange_weak(
                expected, true,
                std::memory_order::acquire, std::memory_order::relaxed
            ))return false;
            if (!readers.load(std::memory_order::acquire) == 0) 
            { 
                writer.store(false, std::memory_order::relaxed); 
                return false; 
            }
            return true;
        }
        
        void lock_shared()
        {
            size_t i = 0;
            while (true)
            {
                if (!writer.load(std::memory_order::acquire))
                {
                    readers.fetch_add(1, std::memory_order::acquire);
                    if (!writer.load(std::memory_order::acquire))return;
                    readers.fetch_add(-1, std::memory_order::relaxed);
                }
                i++;
                if (i > retreatFreq)std::this_thread::yield();
            }
        }
        void unlock_shared() { readers.fetch_add(-1, std::memory_order::release); }
        bool try_lock_shared()
        {
            if (!writer.load(std::memory_order::acquire))
            {
                readers.fetch_add(1, std::memory_order::acquire);
                if (!writer.load(std::memory_order::acquire))return true;
                readers.fetch_add(-1, std::memory_order::relaxed);
            }
            return false;
        }
    };

    using atomic_shared_mutex = atomic_shared_mutex_temp<>;

}