#pragma once
#include "pch.h"
#include "framework.h"

#include "background.h"
#include "labourer_exception.h"
#include "concepts.h"
#include "shared_containers.h"


namespace HYDRA15::Union::labourer
{
    /***************************** 线程池基础 *****************************/
    // 预留任务调度策略的改造空间

    // 定义任务工作的接口，任务细节存储在派生类中
    class mission_base
    {
    public:
        virtual ~mission_base() = default;

        virtual void task() noexcept = 0;
    };
    using mission = std::unique_ptr<mission_base>;

    // 定义线程池的基本行为：提交任务、线程执行任务
    template<typename queue_t>
        requires requires(queue_t q, mission pkg)
    {
        { q.push(pkg) };                            // 应当是阻塞式
        { q.pop() }-> std::convertible_to<mission>; // 应当是阻塞式
        { q.size() }->std::convertible_to<size_t>;
        { q.notify_exit() };                        // 用于结束时使用，通知等待线程应该退出
    }
    class thread_pool : public background
    {
    protected: // 数据
        queue_t queue;
    private:
        std::atomic_bool working = true;

    private: // 后台任务
        virtual void work(background::thread_info& info) override
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
                mis->task();
            }
        }

    public: // 提交接口
        void submit(mission&& mis) { queue.push(std::move(mis)); }

    public: // 构造
        thread_pool() = delete;
        thread_pool(const thread_pool&) = delete;
        thread_pool(thread_pool&&) = delete;
        thread_pool(unsigned int threadCount) :background(threadCount) { background::start(); }
        virtual ~thread_pool()
        {
            working.store(false, std::memory_order_relaxed);
            queue.notify_exit();
            background::wait_for_end();
        }

    public: // 管理接口
        size_t waiting() const { return queue.size(); }
        using background::iterator;
        using background::begin;
        using background::end;
    };

    /***************************** 基本线程池实现 *****************************/
    // 专用于 ThreadLake 的任务实现
    template<typename ret>
    class lake_mission : public mission_base
    {
        std::function<ret()> tsk;
        std::function<void(const ret&)> cb;
        std::promise<ret> prms;
    public:
        virtual ~lake_mission() = default;
        lake_mission(const std::function<ret()>& task, const std::function<void(const ret&)>& callback)
            :tsk(task), cb(callback) {
        }

        virtual void task() noexcept override
        {
            try
            {
                if (!tsk)return;
                if constexpr (std::is_void_v<ret>) { tsk(); if (cb)cb(); prms.set_value(); }
                else { 
                    ret t = tsk(); if (cb)cb(t); 
                    if constexpr (std::is_move_constructible_v<ret>)prms.set_value(std::move(t));
                    else prms.set_value(t);
                }
                return;
            }
            catch (...) { prms.set_exception(std::current_exception()); }
        }

        std::future<ret> get_future() { return prms.get_future(); }
    };


    template<template<typename ...> typename queue_t>
    class thread_lake : public thread_pool<queue_t<mission>>
    {
    public: // 构造
        virtual ~thread_lake() = default;
        thread_lake(unsigned int threadCount) :thread_pool<queue_t<mission>>(threadCount) {}
    public: // 提交任务和回调函数
        template<typename ret>
        auto submit(
            const std::function<ret()>& task,
            const std::function<void(const ret&)>& callback = std::function<void(const ret&)>()
        ) -> std::future<ret> {
            auto pmis = std::make_unique<lake_mission<ret>>(task, callback);
            auto fut = pmis->get_future();
            thread_pool<queue_t<mission>>::submit(std::move(pmis));
            return fut;
        }

        // 提交函数和参数，此方法不支持回调
        template<typename F, typename ... Args>
        requires std::invocable<F,Args...>
        auto submit(F&& f, Args&& ... args)
            -> std::future<std::invoke_result_t<F, Args...>> 
        {
            using ret = std::invoke_result_t<F, Args...>;
            return submit<ret>(
                std::function<ret()>(std::bind(std::forward<F>(f), std::forward<Args>(args)...))
            );
        }
    };

    using ThreadLake = thread_lake<basic_blockable_queue>;
}
