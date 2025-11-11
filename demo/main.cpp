#include "pch.h"

#include "Union/lockless_queue.h"

using namespace  HYDRA15::Union::labourer;


class locked_queue
{
    std::queue<int> queue;
    std::mutex mtx;
    std::condition_variable cv;

public:
    void push(int i)
    {
        std::unique_lock ul{ mtx };
        queue.push(i);
        cv.notify_one();
    }

    int pop()
    {
        std::unique_lock ul{ mtx };
        while (queue.empty())cv.wait(ul);
        int i = queue.front();
        queue.pop();
        return i;
    }

    size_t size() const { return queue.size(); }
};

//lockless_queue<int, 1024 * 1024> queue;
locked_queue queue;
std::atomic<size_t> enqueCount = 0;
std::atomic<size_t> dequeCount = 0;


void push_work()
{
    while (true)
    {
        
        queue.push(500);
        enqueCount.fetch_add(1, std::memory_order_relaxed);
    }
}

void pop_work()
{
    while (true)
    {
        queue.pop();
        std::atomic_thread_fence(std::memory_order_seq_cst);
        dequeCount.fetch_add(1, std::memory_order_relaxed);
    }
}

int main()
{
    std::cout << std::thread::hardware_concurrency() << std::endl;
    for (size_t i = 0; i < std::thread::hardware_concurrency() / 2; i++)
        std::thread(pop_work).detach();
    for (size_t i = 0; i < std::thread::hardware_concurrency() / 2; i++)
        std::thread(push_work).detach();

    auto lastRefresh = std::chrono::steady_clock::now();
    size_t lastEnque = 0;
    size_t lastDeque = 0;
    while (true)
    {
        size_t enque = enqueCount.load(std::memory_order_relaxed);
        size_t deque = dequeCount.load(std::memory_order_relaxed);
        auto now = std::chrono::steady_clock::now();
        auto dur = std::chrono::duration_cast<std::chrono::duration<double>>(now - lastRefresh).count();
        std::cout << std::format("enque {:.4f} tps, deque {:.4f} tps, queue size {}\n", (enque - lastEnque) / dur, (deque - lastDeque) / dur, queue.size());
        lastRefresh = now;
        lastDeque = deque;
        lastEnque = enque;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
