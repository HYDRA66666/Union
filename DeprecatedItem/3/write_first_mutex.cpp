#include "pch.h"
#include "write_first_mutex.h"

namespace HYDRA15::Union::labourer
{
    void write_first_mutex_v1::lock()
    {
        std::unique_lock<std::mutex> lk(mutex);

        waitingWriters++;
        writeCond.wait(lk, [this]() { return activeReaders == 0 && activeWriters == 0; });

        waitingWriters--;
        activeWriters = true;
    }

    void write_first_mutex_v1::unlock()
    {
        std::unique_lock<std::mutex> lk(mutex);

        activeWriters = false;

        if (waitingWriters > 0)
            writeCond.notify_one();
        else
            readCond.notify_all();
    }

    bool write_first_mutex_v1::try_lock()
    {
        std::unique_lock<std::mutex> lk(mutex);

        if (activeReaders == 0 && activeWriters == 0)
        {
            activeWriters = true;
            return true;
        }
        return false;
    }

    void write_first_mutex_v1::lock_shared()
    {
        std::unique_lock<std::mutex> lk(mutex);

        readCond.wait(lk, [this]() { return waitingWriters == 0 && activeWriters == 0; });

        activeReaders++;
    }

    void write_first_mutex_v1::unlock_shared()
    {
        std::unique_lock<std::mutex> lk(mutex);

        activeReaders--;

        if (activeReaders == 0 && waitingWriters > 0)
            writeCond.notify_one();
    }

    bool write_first_mutex_v1::try_lock_shared()
    {
        std::unique_lock<std::mutex> lk(mutex);

        if (waitingWriters == 0 && activeWriters == 0)
        {
            activeReaders++;
            return true;
        }
        return false;
    }



    void write_first_mutex_v2::lock()
    {
        doorLock.lock();
        mtx.lock();
    }

    void write_first_mutex_v2::unlock()
    {
        doorLock.unlock();
        mtx.unlock();
    }

    bool write_first_mutex_v2::try_lock()
    {
        if (!doorLock.try_lock())
            return false;
        if (!mtx.try_lock())
        {
            doorLock.unlock();
            return false;
        }
        return true;
    }

    void write_first_mutex_v2::lock_shared()
    {
        doorLock.lock();
        std::atomic_thread_fence(std::memory_order_seq_cst);
        doorLock.unlock();
        mtx.lock_shared();
    }

    void write_first_mutex_v2::unlock_shared()
    {
        mtx.unlock_shared();
    }

    bool write_first_mutex_v2::try_lock_shared()
    {
        if (!doorLock.try_lock())
            return false;
        std::atomic_thread_fence(std::memory_order_seq_cst);
        doorLock.unlock();
        return mtx.try_lock_shared();
    }
}