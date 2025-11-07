#pragma once
#include "pch.h"
#include "framework.h"


namespace HYDRA15::Union::labourer
{
    // 写优先锁满足条件：
    //    - 使用 std::shared_lock 上锁视为读取操作，使用 std::unique_lock 上锁视为写入操作
    //    - 同一时间允许多个读取操作存在，允许一个写入操作存在，写入操作和读取操作不能同时存在
    //    - 当有写入线程等待时，停止新的读取线程的进入，待已有读取线程都退出后，允许写入线程进入
    //    - 当所有写入线程退出后，允许新的读取线程进入
    // ***** 包含AI编写的代码，并且未经过验证 *****

    /*************************** 性能分析 **************************
    *                               纯写锁          纯读锁
    * 无锁                        4000w tps
    * std::mutex                   200w tps            -
    * std::shared_mutex            250w tps        540w tps
    * write_first_mutex_v1          40w tps         80w tps
    * write_first_mutex_v2          80w tps        140w tps
    * 
    * 测试脚本：
        std::atomic<size_t> count = 0;
        write_first_mutex mtx;

        void mtx_test()
        {
            while (true)
            {
                std::shared_lock ul{ mtx };
                count++;
            }
        }

        void block_service()
        {
            while (true)
            {
                std::this_thread::sleep_for(std::chrono::seconds(5));
                std::unique_lock ul{ mtx };
                std::cout << "Block service entered\n";
                std::this_thread::sleep_for(std::chrono::seconds(5));
                std::cout << "Block service exited\n";
                }
        }

        int main()
        {
            for (size_t i = 0; i < std::thread::hardware_concurrency() - 1; i++)
                std::thread(mtx_test).detach();
            std::thread(block_service).detach();
                auto lastRefresh = std::chrono::steady_clock::now();
            size_t lastCount = 0;
            while (true)
            {
                size_t x = count;
                auto now = std::chrono::steady_clock::now();
                auto dur = std::chrono::duration_cast<std::chrono::duration<double>>(now - lastRefresh).count();
                std::cout << std::format("{:.4f}tps, {:08X}\n", (x - lastCount) / dur, x);
                lastRefresh = now;
                lastCount = x;
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    */
    class write_first_mutex_v1
    {
        std::mutex mutex;
        std::condition_variable readCond;
        std::condition_variable writeCond;
        unsigned int activeReaders = 0;
        unsigned int waitingWriters = 0;
        bool activeWriters = false;

    public:
        write_first_mutex_v1() = default;
        write_first_mutex_v1(const write_first_mutex_v1&) = delete;
        write_first_mutex_v1(write_first_mutex_v1&&) = default;
        ~write_first_mutex_v1() = default;

        void lock();
        void unlock();
        bool try_lock();

        void lock_shared();
        void unlock_shared();
        bool try_lock_shared();
    };

    class write_first_mutex_v2
    {
        std::mutex doorLock;
        std::shared_mutex mtx;

    public:
        void lock();
        void unlock();
        bool try_lock();

        void lock_shared();
        void unlock_shared();
        bool try_lock_shared();
    };

    using write_first_mutex = write_first_mutex_v2;
}