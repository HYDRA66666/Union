#include "pch.h"
#include "write_first_mutex.h"

namespace HYDRA15::Foundation::labourer
{
    void write_first_mutex::lock()
    {
        std::unique_lock<std::mutex> lk(mutex);

        waitingWriters++;
        writeCond.wait(lk, [this]() { return activeReaders == 0 && activeWriters == 0; });

        waitingWriters--;
        activeWriters = true;
    }

    void write_first_mutex::unlock()
    {
        std::unique_lock<std::mutex> lk(mutex);

        activeWriters = false;

        if (waitingWriters > 0)
            writeCond.notify_one();
        else
            readCond.notify_all();
    }

    bool write_first_mutex::try_lock()
    {
        std::unique_lock<std::mutex> lk(mutex);

        if (activeReaders == 0 && activeWriters == 0)
        {
            activeWriters = true;
            return true;
        }
        return false;
    }

    void write_first_mutex::lock_shared()
    {
        std::unique_lock<std::mutex> lk(mutex);

        readCond.wait(lk, [this]() { return waitingWriters == 0 && activeWriters == 0; });

        activeReaders++;
    }

    void write_first_mutex::unlock_shared()
    {
        std::unique_lock<std::mutex> lk(mutex);

        activeReaders--;

        if (activeReaders == 0 && waitingWriters > 0)
            writeCond.notify_one();
    }

    bool write_first_mutex::try_lock_shared()
    {
        std::unique_lock<std::mutex> lk(mutex);

        if (waitingWriters == 0 && activeWriters == 0)
        {
            activeReaders++;
            return true;
        }
        return false;
    }
}