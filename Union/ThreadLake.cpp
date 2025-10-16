#include "pch.h"
#include "ThreadLake.h"

namespace HYDRA15::Union::labourer
{
    void ThreadLake::work(background::thread_info& info)
    {
        package taskPkg;
        info.thread_state = background::thread_info::state::idle;

        while (true)
        {
            // 取任务
            info.thread_state = background::thread_info::state::waiting;
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                if (working && taskQueue.empty())
                    queueCv.wait(lock, [this] { return !taskQueue.empty() || !working; });
                if (!working)	// 如果工作结束，退出循环
                    return;
                taskPkg = std::move(taskQueue.front());
                taskQueue.pop();
            }

            // 执行任务
            info.thread_state = background::thread_info::state::working;
            info.workStartTime = std::chrono::steady_clock::now();
            try
            {
                if (taskPkg.content)
                    taskPkg.content();
                if (taskPkg.callback)
                    taskPkg.callback();
            }
            catch (...) {}
        }

    }

    ThreadLake::ThreadLake(unsigned int threadCount, size_t tskQueMaxSize)
        : background(threadCount), tskQueMaxSize(tskQueMaxSize)
    {
        working = true;
        start();
    }

    ThreadLake::~ThreadLake()
    {
        working = false;
        queueCv.notify_all(); // 通知所有等待的线程
        wait_for_end(); // 等待所有线程结束
    }

    std::future<void> ThreadLake::submit(const std::function<void()>& content, const std::function<void()>& callback)
    {
        auto tsk = std::make_shared<std::packaged_task<void()>>(content);

        std::lock_guard<std::mutex> lock(queueMutex);
        taskQueue.push(package{ [tsk]() {(*tsk)(); } ,callback });
        queueCv.notify_one(); // 通知一个等待的线程

        return tsk->get_future();
    }

    void ThreadLake::submit(const package& taskPkg)
    {
        if (!taskPkg.content)
            throw exceptions::labourer::EmptyTask();

        if (tskQueMaxSize != 0 && taskQueue.size() >= tskQueMaxSize) // 队列已满
            throw exceptions::labourer::TaskQueueFull();

        std::lock_guard<std::mutex> lock(queueMutex);
        taskQueue.push(taskPkg);
        queueCv.notify_one(); // 通知一个等待的线程
    }
}
