#pragma once
#include "pch.h"
#include "framework.h"


namespace HYDRA15::Foundation::labourer
{
    // 写优先锁满足条件：
    //    - 使用 std::shared_lock 上锁视为读取操作，使用 std::unique_lock 上锁视为写入操作
    //    - 同一时间允许多个读取操作存在，允许一个写入操作存在，写入操作和读取操作不能同时存在
    //    - 当有写入线程等待时，停止新的读取线程的进入，待已有读取线程都退出后，允许写入线程进入
    //    - 当所有写入线程退出后，允许新的读取线程进入
    // ***** 包含AI编写的代码，并且未经过验证 *****
    class write_first_mutex
    {
        std::mutex mutex;
        std::condition_variable readCond;
        std::condition_variable writeCond;
        unsigned int activeReaders = 0;
        unsigned int waitingWriters = 0;
        bool activeWriters = false;

    public:
        write_first_mutex() = default;
        ~write_first_mutex() = default;

        void lock();
        void unlock();
        bool try_lock();

        void lock_shared();
        void unlock_shared();
        bool try_lock_shared();



    };
}