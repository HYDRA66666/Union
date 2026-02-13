export module HYDRA15.Union.AtomicMutex;

import std;
import HYDRA15.Union.exceptions;

namespace HYDRA15::Union
{
    /**
    * @brief 使用原子变量实现的读写锁，自旋等待，适用于短临界区或读多写少
    * 支持锁升级，此特性对于一般读写锁操作几乎没有额外开销
    * 限制总读锁数量为 0xFFFFFFFF
    *
    * 性能测试：
    *                           debug       release
    *  无锁（纯原子计数器）  4450w tps     6360w tps
    *  std::shared_mutex      480w tps      670w tps
    *  AtomicSharedMutex    920w tps     1470w tps
    */
    export template<bool shared = true, size_t retreatFreq = 32>
    class basic_atomic_mutex
    {
    private:
        std::atomic_bool writer = false;
        std::atomic_bool upgraded = false;
        std::atomic<std::uint64_t> readers = 0;

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
            if(shared)
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
            if (shared && !readers.load(std::memory_order::acquire) == 0)
                return false;           // 有读锁
            if (!writer.compare_exchange_strong(
                expected, true,
                std::memory_order::acquire, std::memory_order::relaxed
            ))return false;             // 获取写锁失败
            if (shared && !readers.load(std::memory_order::acquire) == 0)
            {
                writer.store(false, std::memory_order::release);
                return false;           // 获取写锁成功但有读锁
            }
            return true;
        }

        void lock_shared() requires(shared)
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

        void unlock_shared() requires(shared)
        { 
            readers.fetch_sub(1, std::memory_order::release); 
        }

        bool try_lock_shared() requires(shared)
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
        void upgrade() requires(shared)
        {
            readers.fetch_add(0xFFFFFFFF, std::memory_order::acquire);
            size_t i = 0;
            while (true)
            {
                bool expected = false;
                if (upgraded.compare_exchange_strong(
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

        void downgrade() requires(shared)
        {
            readers.fetch_sub(0xFFFFFFFF, std::memory_order::release);
            upgraded.store(false, std::memory_order::release);
        }

        bool try_upgrade() requires(shared)
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

    export using AtomicSharedMutex = basic_atomic_mutex<>;
    export using AtomicMutex = basic_atomic_mutex<false>;
}