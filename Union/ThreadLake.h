#pragma once
#include "pch.h"
#include "framework.h"

#include "background.h"
#include "labourer_exception.h"
#include "concepts.h"
#include "basic_blockable_queue.h"


namespace HYDRA15::Union::labourer
{
    using mission = std::pair<std::function<void()>, std::function<void()>>;

    // 线程池传入的模板参数为可阻塞、线程安全的队列类型，其调度行为由队列类型确定
    // 当析构时，正在执行的任务会继续执行完成，队列中剩余的任务会被丢弃
    template<template<typename ...> typename Q>
        requires requires(Q<mission> q, mission pkg) 
            { 
                { q.push(pkg) };                            // 应当是阻塞式
                { q.pop() }-> std::convertible_to<mission>; // 应当是阻塞式
                { q.size() }->std::convertible_to<size_t>;
                { q.notify_exit() };                        // 用于结束时使用，通知等待线程应该退出
            }
    class thread_lake : public background
    {
    private:
        Q<mission> queue;

    private:
        //后台任务
        std::atomic_bool working = true;
        virtual void work(background::thread_info& info) override;

    public:
        thread_lake() = delete;
        thread_lake(const thread_lake&) = delete;
        thread_lake(thread_lake&&) = delete;
        thread_lake(unsigned int threadCount);
        ~thread_lake();


        // 提交任务接口
        // 基本方法：提交任务包
        void submit(const mission&);

        // 提交任务函数和回调函数
        template<typename ret>
        auto submit(
            const std::function<ret()>& task,
            const std::function<void(std::shared_future<ret>)>& callback = std::function<void(std::shared_future<ret>)>()
        ) -> std::shared_future<ret>;

        // 提交函数和参数，此方法不支持回调
        template<typename F, typename ... Args>
            requires std::invocable<F,Args...>
        auto submit(F&& f, Args&& ... args) 
            -> std::shared_future<std::invoke_result_t<F, Args...>>;


        size_t waiting() const; // 等待中的任务数

    };

    using ThreadLake = thread_lake<basic_blockable_queue>;



    template<template<typename ...> typename Q>
        requires requires(Q<mission> q, mission pkg)
    {
        { q.push(pkg) };
        { q.pop() }-> std::convertible_to<mission>;
        { q.size() }->std::convertible_to<size_t>;
        { q.notify_exit() };    // 用于结束时使用，通知等待线程应该退出
    }
    inline void thread_lake<Q>::work(background::thread_info& info)
    {
        mission mis;
        info.thread_state = background::thread_info::state::idle;

        while (working.load(std::memory_order_relaxed))
        {
            // 取任务
            info.thread_state = background::thread_info::state::waiting;
            mis = queue.pop();

            // 执行任务
            info.thread_state = background::thread_info::state::working;
            info.workStartTime = std::chrono::steady_clock::now();
            try
            {
                if (mis.first) mis.first();
                if (mis.second)mis.second();
            }
            catch (...) {}
        }
    }

    template<template<typename ...> typename Q>
        requires requires(Q<mission> q, mission pkg)
    {
        { q.push(pkg) };
        { q.pop() }-> std::convertible_to<mission>;
        { q.size() }->std::convertible_to<size_t>;
        { q.notify_exit() };    // 用于结束时使用，通知等待线程应该退出
    }
    inline thread_lake<Q>::thread_lake(unsigned int threadCount)
        :background(threadCount)
    {
        background::start();
    }

    template<template<typename ...> typename Q>
        requires requires(Q<mission> q, mission pkg)
    {
        { q.push(pkg) };
        { q.pop() }-> std::convertible_to<mission>;
        { q.size() }->std::convertible_to<size_t>;
        { q.notify_exit() };    // 用于结束时使用，通知等待线程应该退出
    }
    inline thread_lake<Q>::~thread_lake()
    {
        working.store(false, std::memory_order_relaxed);
        queue.notify_exit();
        std::atomic_thread_fence(std::memory_order_release);
        background::wait_for_end();
    }

    template<template<typename ...> typename Q>
        requires requires(Q<mission> q, mission pkg)
    {
        { q.push(pkg) };
        { q.pop() }-> std::convertible_to<mission>;
        { q.size() }->std::convertible_to<size_t>;
        { q.notify_exit() };    // 用于结束时使用，通知等待线程应该退出
    }
    inline void thread_lake<Q>::submit(const mission& mis)
    {
        queue.push(mis);
    }

    template<template<typename ...> typename Q>
        requires requires(Q<mission> q, mission pkg)
    {
        { q.push(pkg) };
        { q.pop() }-> std::convertible_to<mission>;
        { q.size() }->std::convertible_to<size_t>;
        { q.notify_exit() };    // 用于结束时使用，通知等待线程应该退出
    }
    template<typename ret>
    auto thread_lake<Q>::submit(
        const std::function<ret()>& task,
        const std::function<void(std::shared_future<ret>)>& callback
    ) -> std::shared_future<ret>
    {
        auto ppkgedTask = std::make_shared<std::packaged_task<ret()>>(
            [task]()->ret {if (task)return task(); }
        );
        auto sft = ppkgedTask->get_future().share();
        auto ppkgedCallback = std::make_shared<std::packaged_task<void()>>(
            [callback, sft]() {if (callback)callback(sft); }
        );
        
        submit(std::move(mission{
            [p = std::move(ppkgedTask)]() {(*p)(); },
            [p = std::move(ppkgedCallback)]() {(*p)(); }
            }));

        return sft;
    }

    template<template<typename ...> typename Q>
        requires requires(Q<mission> q, mission pkg)
    {
        { q.push(pkg) };
        { q.pop() }-> std::convertible_to<mission>;
        { q.size() }->std::convertible_to<size_t>;
        { q.notify_exit() };    // 用于结束时使用，通知等待线程应该退出
    }
    template<typename F, typename ...Args>
        requires std::invocable<F, Args...>
    inline auto thread_lake<Q>::submit(F&& f, Args&& ...args) 
        -> std::shared_future<std::invoke_result_t<F, Args ...>>
    {
        using ret = std::invoke_result_t<F, Args...>;
        return submit<ret>(
            std::function<ret()>(std::bind(std::forward<F>(f), std::forward<Args>(args)...))
        );
    }

    template<template<typename ...> typename Q>
        requires requires(Q<mission> q, mission pkg)
    {
        { q.push(pkg) };
        { q.pop() }-> std::convertible_to<mission>;
        { q.size() }->std::convertible_to<size_t>;
        { q.notify_exit() };    // 用于结束时使用，通知等待线程应该退出
    }
    inline size_t thread_lake<Q>::waiting() const
    {
        return queue.size();
    }
}
