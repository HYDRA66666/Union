#include "pch.h"

#include "Union/lockless_queue.h"
#include "Union/ThreadLake.h"

using namespace  HYDRA15::Union::labourer;

template<typename T>
using lockless = lockless_queue<T, 1024 * 1024>;

//thread_lake<lockless> thrlake(4);
ThreadLake thrlake(4);
std::atomic<size_t> submitCounter;
std::atomic<size_t> workCounter;

size_t working_work(size_t i)
{
    workCounter.fetch_add(1, std::memory_order_relaxed);
    return i + 1;
}

void submit_work()
{
    while (true)
    {
        thrlake.submit(working_work, submitCounter.fetch_add(1, std::memory_order_relaxed));
    }
}

int main()
{
    for (size_t i = 0; i < 4; i++)
        std::thread(submit_work).detach();

    auto lastRefresh = std::chrono::steady_clock::now();
    size_t lastSbmtCnt = 0;
    size_t lastWrkCnt = 0;
    while (true)
    {
        auto now = std::chrono::steady_clock::now();
        size_t nowSub = submitCounter.load(std::memory_order_relaxed);
        size_t nowWrk = workCounter.load(std::memory_order_relaxed);
        auto dur = std::chrono::duration_cast<std::chrono::duration<double>>(now - lastRefresh).count();
        std::cout << std::format("submit {:.4f} tps, work {:.4f} tps\n", (nowSub - lastSbmtCnt) / dur, (nowWrk - lastWrkCnt) / dur);
        lastRefresh = now;
        lastSbmtCnt = nowSub;
        lastWrkCnt = nowWrk;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}


