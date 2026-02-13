#pragma once
#include "pch.h"
#include "framework.h"

#include "concepts.h"
#include "lib_exceptions.h"

namespace HYDRA15::Union::labourer
{
    template<size_t retreatFreq = 32>
    class basic_atomic_mutex
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

    using atomic_mutex = basic_atomic_mutex<>;


    // 使用原子变量实现的读写锁，自旋等待，适用于短临界区或读多写少
    // 支持锁升级，此特性对于一般读写锁操作几乎没有额外开销
    // 限制总读锁数量为 0xFFFFFFFF
    // 
    // 性能测试：
    //                           debug       release
    //  无锁（纯原子计数器）  4450w tps     6360w tps
    //  std::shared_mutex      480w tps      670w tps
    //  atomic_shared_mutex    920w tps     1470w tps
    template<size_t retreatFreq = 32>
    class basic_atomic_shared_mutex
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
                    std::memory_order::acq_rel, std::memory_order::relaxed
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

        bool try_upgrade()
        {
            readers.fetch_add(0xFFFFFFFF, std::memory_order::acquire);

            bool expected = false;
            if (!upgraded.compare_exchange_strong(
                expected, true,
                std::memory_order::acq_rel, std::memory_order::relaxed))
            {
                readers.fetch_sub(0xFFFFFFFF, std::memory_order::relaxed);
                return false;
            }

            if ((readers.load(std::memory_order::relaxed) & 0xFFFFFFFFu) != 1u)
            {
                upgraded.store(false, std::memory_order::relaxed);
                readers.fetch_sub(0xFFFFFFFF, std::memory_order::relaxed);
                return false;
            }

            return true;
        }
    };

    using atomic_shared_mutex = basic_atomic_shared_mutex<>;


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


    // 在同一线程重复上锁时不会重复操作底层锁的读写锁
    // 有效解决读写锁在重入时的性能和死锁问题
    // 要求解锁必须按照上锁的逆序进行，否则行为未定义
    template<typename L>
    class thread_mutex
    {
    private:
        struct local_state
        {
            std::weak_ptr<void> ctrl;   // 用于锁实例销毁时清理线程局部状态
            size_t readers = 0;
            size_t writers = 0;
            bool upgraded = 0;
            local_state(const std::shared_ptr<void>& c) :ctrl(c) {}
        };

    private:
        static thread_local inline std::unordered_map<const void*, local_state> states{};

        L mtx;

        std::shared_ptr<void> ctrl;

    private:
        local_state& state() 
        { 
            if (!states.contains(static_cast<const void*>(this)))
            {
                states.emplace(static_cast<const void*>(this), ctrl);
                // 每次新建状态时清理已过期的状态
                for (auto it = states.begin(); it != states.end(); )
                {
                    if (it->second.ctrl.expired())
                        it = states.erase(it);
                    else
                        ++it;
                }
            }
            return states.at(static_cast<const void*>(this));
        }

    public:
        void lock()
        {
            auto& s = state();
            auto beforeWriters = s.writers;
            s.writers++;

            if (beforeWriters > 0) return; // 已持有写锁，重入仅计数

            if (s.readers > 0)
                if constexpr (framework::upgrade_lockable<L>)
                {
                    // 有本线程的读锁：需要升级到写锁（阻塞式）
                    mtx.upgrade();
                    s.upgraded = true;
                    return;
                }
                else throw exceptions::common("operation requires upgradeable lock");
            
            // 首次上锁
            mtx.lock();
            return;
        }

        void unlock()
        {
            auto& s = state();
            s.writers--;

            if (s.writers > 0) return; // 仍有重入写锁，延迟释放

            // 最后一个写锁释放：根据是否通过 upgrade 获得决定降级或直接释放
            if (s.upgraded)
                if constexpr (framework::upgrade_lockable<L>)
                {
                    // 之前由读升级到写：降级恢复为读（底层会处理 readers 计数）
                    mtx.downgrade();
                    s.upgraded = false;
                    return;
                }

            mtx.unlock();
        }

        bool try_lock()
        {
            auto& s = state();

            if (s.writers > 0)
            {
                ++s.writers;
                return true;
            }

            if (s.readers > 0)
                if constexpr (requires(L l) { { l.try_upgrade() }->std::same_as<bool>; })
                {
                    if (mtx.try_upgrade())
                    {
                        s.writers++;
                        s.upgraded = true;
                        return true;
                    }
                    else return false;
                }
                else return false;

            if (mtx.try_lock())
            {
                s.writers++;
                return true;
            }
            return false;
        }

        void lock_shared()
        {
            auto& s = state();
            auto beforeReaders = s.readers;
            s.readers++;

            if (s.writers > 0)
                // 已持有写锁，本次 shared 只是逻辑计数
                return;

            // 首次上锁
            if (beforeReaders == 0)
                mtx.lock_shared(); 
        }

        void unlock_shared()
        {
            auto& s = state();
            s.readers--;

            if (s.writers > 0)
                // 在写锁下的读只是局部计数
                return;

            if (s.readers == 0)
                // 最后一个本线程的读释放底层读锁
                mtx.unlock_shared();
        }

        bool try_lock_shared()
        {
            auto& s = state();

            if (s.writers > 0)
            {
                s.readers++;
                return true;
            }

            if (s.readers == 0)
            {
                if (mtx.try_lock_shared())
                {
                    s.readers++;
                    return true;
                }
                return false;
            }

            // 已有本线程读锁，重入直接成功
            s.readers++;
            return true;
        }
    
    public:
        thread_mutex() :ctrl(std::make_shared<char>(0)) {}
    };


    template<typename L>
        requires framework::upgrade_lockable<L>
    class upgrade_lock
    {
    private:
        L& smtx;

    public:
        upgrade_lock(L& m) :smtx(m) { smtx.upgrade(); }
        ~upgrade_lock() { smtx.downgrade(); }
    };
}
