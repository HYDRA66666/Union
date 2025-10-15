#pragma once
#include "pch.h"
#include "framework.h"

#include "Background.h"
#include "labourer_exception.h"


namespace HYDRA15::Union::labourer
{

    // 线程池
    class ThreadLake :background
    {
        // 任务和任务包定义
    public:
        struct package
        {
            std::function<void()> content;
            std::function<void()> callback;	// 任务完成后的回调
        };

    private:
        // 回调函数壳
        template<typename ret_type>
        void callback_shell(std::function<void(ret_type)> callback, std::shared_future<ret_type> sfut)
        {
            try { callback(sfut.get()); }
            catch (...) { return; }
        }

        //任务队列
    private:
        std::queue <package> taskQueue; //任务队列
        const size_t tskQueMaxSize = 0; //任务队列最大大小，0表示无限制
        std::mutex queueMutex;
        std::condition_variable queueCv;

        //后台任务
    private:
        bool working = false;
        virtual void work(background::thread_info& info) override;

        //接口
    public:
        ThreadLake(unsigned int threadCount, size_t tskQueMaxSize = 0);
        ThreadLake() = delete;
        ThreadLake(const ThreadLake&) = delete;
        ThreadLake(ThreadLake&&) = delete;
        virtual ~ThreadLake();

        //提交任务
        // 方法1：提交任务函数 std::function 和回调函数 std::function，推荐使用此方法
        template<typename ret_type>
        auto submit(std::function<ret_type()>& content, std::function<void(ret_type)> callback = std::function<void(ret_type)>())
            -> std::shared_future<ret_type>
        {
            if (!content)
                throw exceptions::labourer::EmptyTask();

            auto pkgedTask = std::make_shared<std::packaged_task<ret_type()>>(content);
            auto sfut = pkgedTask->get_future().share();

            // 插入任务包
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                if (tskQueMaxSize != 0 && taskQueue.size() >= tskQueMaxSize) // 队列已满
                    throw exceptions::labourer::TaskQueueFull();
                taskQueue.push(
                    {
                        std::function<void()>([pkgedTask] { (*pkgedTask)(); }),
                        callback ? std::function<void()>(callback_shell, callback, sfut) : std::function<void()>()
                    }
                );
                queueCv.notify_one();
            }

            return sfut;
        }

        //方法2：直接提交任务包
        void submit(const package& taskPkg);

        // 方法3：提交裸函数指针和参数，不建议使用此方法，仅留做备用
        template<typename F, typename ... Args>
        auto submit(F&& f, Args &&...args)
            -> std::shared_future<typename std::invoke_result<F, Args...>::type>
        {
            using return_type = typename std::invoke_result<F, Args...>::type;

            auto pkgedTask =
                std::make_shared<std::packaged_task<return_type()>>(
                    std::bind(std::forward<F>(f), std::forward<Args>(args)...)
                );

            // 插入任务包
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                if (tskQueMaxSize != 0 && taskQueue.size() >= tskQueMaxSize) // 队列已满
                    throw exceptions::labourer::TaskQueueFull();

                taskQueue.push(
                    {
                        std::function<void()>([pkgedTask] { (*pkgedTask)(); }),
                        std::function<void()>()
                    }
                );
                queueCv.notify_one();
            }

            return pkgedTask->get_future().share();
        }

        // 迭代器访问每一个线程信息
        using background::iterator;
        using background::begin;
        using background::end;
    };
}
