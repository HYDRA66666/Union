export module HYDRA15.Union.ThreadLake;

import std;
import HYDRA15.Union.astring;
import HYDRA15.Union.log;

namespace HYDRA15::Union
{
    export class ThreadLake
    {
    public:
        ThreadLake() = delete;
        ThreadLake(const ThreadLake&) = delete;
        ThreadLake(ThreadLake&&) = delete;
        ThreadLake(unsigned int threadCnt, const std::string& name = "ThreadLake")
        {
            threads.reserve(threadCnt);
            for (unsigned int i = 0; i < threadCnt; ++i)
                threads.emplace_back(&ThreadLake::work, this, logger(
                    std::format("{} [working thread {}]", name, i)
                ));
            startLatch.count_down();
        }
        ~ThreadLake()
        {
            working = false;
            tasksCv.notify_all();
            for (auto& t : threads)
                if (t.joinable()) t.join();
        }

        template<typename ReturnType>
        std::future<ReturnType> submit(std::function<ReturnType()> func)
        {
            std::shared_ptr<std::packaged_task<ReturnType()>> task = std::make_shared<std::packaged_task<ReturnType()>>(
                [func = std::move(func)]() { return func(); }
            );
            auto result = task->get_future();
            {
                std::lock_guard lock(tasksMutex);
                tasks.emplace([task = std::move(task)]() mutable {
                    (*task)();
                });
            }
            tasksCv.notify_one();
            return result;
        }

        std::future<void> submit(std::function<void()> func)
        {
            std::shared_ptr<std::packaged_task<void()>> task = std::make_shared<std::packaged_task<void()>>(
                [func = std::move(func)]() { func(); }
            );
            auto result = task->get_future();
            {
                std::lock_guard lock(tasksMutex);
                tasks.emplace([task = std::move(task)]() mutable {
                    (*task)();
                });
            }
            tasksCv.notify_one();
            return result;
        }

    private:
        std::atomic_bool working{ true };
        std::latch startLatch{ 1 };
        std::vector<std::jthread> threads;
        // 任务队列
        std::queue<std::function<void()>> tasks;
        std::mutex tasksMutex;
        std::condition_variable_any tasksCv;

        void work(logger logger)
        {
            startLatch.wait();
            while (working.load(std::memory_order::relaxed) || !tasks.empty())
            {
                std::function<void()> task;
                {
                    std::unique_lock lock(tasksMutex);
                    tasksCv.wait(lock, [this] { return !tasks.empty() || !working; });
                    if (!working && tasks.empty()) return;
                    task = std::move(tasks.front());
                    tasks.pop();
                }
                if (task) task();
            }
        }
    };


}