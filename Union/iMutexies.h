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


    // 使用原子变量实现的读写锁，自旋等待，适用于短临界区或读多写少
    // 支持锁升级，此特性对于一般读写锁操作没有额外开销
    // 限制总读锁数量为 0xFFFFFFFF
    // 
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
        std::atomic_bool upgraded = false;
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
                if (i >= retreatFreq) { i = 0; std::this_thread::yield(); }
            }
            while (true)
            {
                if (readers.load(std::memory_order::acquire) == 0)break;
                i++;
                if (i >= retreatFreq) { i = 0; std::this_thread::yield(); }
            }
        }

        void unlock() { writer.store(false, std::memory_order::release); }

        bool try_lock()
        {
            bool expected = writer.load(std::memory_order::acquire);
            if (expected)return false;  // 已有写锁
            if (!readers.load(std::memory_order::acquire) == 0)
                return false;           // 有读锁
            if(!writer.compare_exchange_strong(
                expected, true,
                std::memory_order::acquire, std::memory_order::relaxed
            ))return false;             // 获取写锁失败
            return true;
        }
        
        void lock_shared()
        {
            size_t i = 0;
            while (true)
            {
                if (!writer.load(std::memory_order::acquire) && !upgraded.load(std::memory_order::acquire))
                {
                    readers.fetch_add(1, std::memory_order::acquire);
                    if (!writer.load(std::memory_order::acquire) && !upgraded.load(std::memory_order::acquire))return;
                    readers.fetch_sub(1, std::memory_order::relaxed);
                }
                i++;
                if (i >= retreatFreq) { i = 0; std::this_thread::yield(); }
            }
        }

        void unlock_shared() { readers.fetch_sub(1, std::memory_order::release); }

        bool try_lock_shared()
        {
            if (!writer.load(std::memory_order::acquire) && !upgraded.load(std::memory_order::acquire))
            {
                readers.fetch_add(1, std::memory_order::acquire);
                if (!writer.load(std::memory_order::acquire) && !upgraded.load(std::memory_order::acquire))return true;
                readers.fetch_sub(1, std::memory_order::relaxed);
            }
            return false;
        }

        // 升级和降级。upgrade 的返回值应当传递给 downgrade 的 before 参数。
        void upgrade()
        {
            readers.fetch_add(0xFFFFFFFF, std::memory_order::acquire);
            size_t i = 0;
            while (true)
            {
                bool expected = false;
                if(upgraded.compare_exchange_strong(
                    expected, true,
                    std::memory_order::acquire, std::memory_order::relaxed
                ))break;
                i++;
                if (i >= retreatFreq) { i = 0; std::this_thread::yield(); }
            }
            while (true)
            {
                if ((readers.load(std::memory_order_acquire) & 0xFFFFFFFF) == 0)break;
                i++;
                if (i >= retreatFreq) { i = 0; std::this_thread::yield(); }
            }
        }

        void downgrade()
        {
            readers.fetch_sub(0xFFFFFFFF, std::memory_order::release);
            upgraded.store(false, std::memory_order::release);
        }
    };

    using atomic_shared_mutex = atomic_shared_mutex_temp<>;



    // 将在多次失败之后回退到系统调度的混合型互斥锁
    template<size_t retreatFreq = 32>
    class mixed_mutex_temp
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
                lck.wait(true, std::memory_order::relaxed);
            }
        }

        void unlock() { lck.store(false, std::memory_order::release); lck.notify_one(); }

        bool try_lock()
        {
            bool expected = false;
            return lck.compare_exchange_strong(expected, true, std::memory_order::acquire);
        }
    };

    using mixed_mutex = mixed_mutex_temp<>;

    template<size_t retreatFreq = 32>
    class mixed_shared_mutex_temp
    {
    private:
        std::atomic_bool writer = false;
        std::atomic_bool upgraded = false;
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
                if (i >= retreatFreq)
                    writer.wait(true, std::memory_order::relaxed);
            }
            i = 0;
            while (true)
            {
                size_t old;
                if ((old = readers.load(std::memory_order::acquire)) == 0)break;
                i++;
                if (i >= retreatFreq)
                    readers.wait(old, std::memory_order::relaxed);
            }
        }

        void unlock() { writer.store(false, std::memory_order::release); writer.notify_all(); }

        bool try_lock()
        {
            bool expected = writer.load(std::memory_order::acquire);
            if (expected)return false;  // 已有写锁
            if (!readers.load(std::memory_order::acquire) == 0)
                return false;           // 有读锁
            if (!writer.compare_exchange_strong(
                expected, true,
                std::memory_order::acquire, std::memory_order::relaxed
            ))return false;             // 获取写锁失败
            return true;
        }

        void lock_shared()
        {
            size_t i = 0;
            while (true)
            {
                if (!writer.load(std::memory_order::acquire) && !upgraded.load(std::memory_order::acquire))
                {
                    readers.fetch_add(1, std::memory_order::acquire);
                    if (!writer.load(std::memory_order::acquire) && !upgraded.load(std::memory_order::acquire))return;
                    readers.fetch_sub(1, std::memory_order::relaxed);
                    readers.notify_all();
                }
                i++;
                if (i > retreatFreq)
                {
                    writer.wait(true, std::memory_order::relaxed);
                    upgraded.wait(true, std::memory_order::relaxed);
                }
            }
        }

        void unlock_shared() { readers.fetch_sub(1, std::memory_order::release); readers.notify_all(); }

        bool try_lock_shared()
        {
            if (!writer.load(std::memory_order::acquire) && !upgraded.load(std::memory_order::acquire))
            {
                readers.fetch_add(1, std::memory_order::acquire);
                if (!writer.load(std::memory_order::acquire) && !upgraded.load(std::memory_order::acquire))return true;
                readers.fetch_sub(1, std::memory_order::relaxed);
                readers.notify_all();
            }
            return false;
        }

        // 升级和降级。upgrade 的返回值应当传递给 downgrade 的 before 参数。
        void upgrade()
        {
            readers.fetch_add(0xFFFFFFFF, std::memory_order::acquire);
            size_t i = 0;
            while (true)
            {
                bool expected = false;
                if (upgraded.compare_exchange_strong(
                    expected, true,
                    std::memory_order::acquire, std::memory_order::relaxed
                ))break;
                i++;
                if (i >= retreatFreq)
                    upgraded.wait(true, std::memory_order::relaxed);
            }
            i = 0;
            while (true)
            {
                size_t old;
                if (((old = readers.load(std::memory_order_acquire)) & 0xFFFFFFFF) == 0)break;
                i++;
                if (i >= retreatFreq)
                    readers.wait(old, std::memory_order::relaxed);
            }
        }

        void downgrade()
        {
            readers.fetch_sub(0xFFFFFFFF, std::memory_order::release);
            upgraded.store(false, std::memory_order::release);
            readers.notify_all();
            upgraded.notify_all();
        }
    };

    using mixed_shared_mutex = mixed_shared_mutex_temp<>;


    template<typename L>
        requires requires(L l) { l.upgrade(); l.downgrade(); }
    class upgrade_lock
    {
    private:
        L& smtx;

    public:
        upgrade_lock(L& m) :smtx(m) { smtx.upgrade(); }
        ~upgrade_lock() { smtx.downgrade(); }
    };
}
