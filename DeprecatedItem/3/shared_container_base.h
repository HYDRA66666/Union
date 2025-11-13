#pragma once
#include "pch.h"
#include "framework.h"

#include "concepts.h"

namespace HYDRA15::Union::labourer
{
    // 在调用容器接口时上锁
    //    - 标准调用不使用任何锁操作
    //    - 允许多个共享调用同时进行，共享调用进行时禁止独占调用进行
    //    - 独占调用进行时禁止任何其他调用进行
    //    - 独占调用和共享调用的准入调度由锁的类型决定
    // 模板参数：
    //   - Container: 容器类型
    //   - Lock: 锁类型，必须满足 std::mutex 的接口
    // 调用使用示例：
    //   - 无重载成员函数：call(&Container::func, args...)
    //   - 有重载成员函数：call(static_cast<Ret (Container::*)(Args&&...)>(&Container::func), args...)，须在static_cast中指定函数指针重载的版本
    template<class container, class container_lock>
        requires framework::lockable<container_lock>
    class shared_container_base
    {
        container container;

    protected:
        container_lock containerLock;  // 将锁对象暴露给外部，以实现更高级的操作

    public:
        virtual ~shared_container_base() = default;

        // 标准调用
        template<typename F, typename... Args>
        decltype(auto) call(F&& f, Args&&... args)
        {
            return std::invoke(std::forward<F>(f), container, std::forward<Args>(args)...);
        }

        template<typename F, typename... Args>
        decltype(auto) static_call(F&& f, Args&&... args)
        {
            return std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
        }

        // 锁调用
        template<typename F, typename... Args>
        decltype(auto) call_locked(F&& f, Args&&... args)
        {
            std::lock_guard<container_lock> guard(containerLock);
            return std::invoke(std::forward<F>(f), container, std::forward<Args>(args)...);
        }

        template<typename F, typename... Args>
        decltype(auto) static_call_locked(F&& f, Args&&... args)
        {
            std::lock_guard<container_lock> guard(containerLock);
            return std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
        }

        // 共享调用
        template<typename F, typename... Args>
        decltype(auto) call_shared(F&& f, Args&&... args)
        {
            if constexpr (framework::shared_lockable<container_lock>)
                std::shared_lock<container_lock> guard(containerLock);
            else
                std::lock_guard<container_lock> guard(containerLock);
            return std::invoke(std::forward<F>(f), container, std::forward<Args>(args)...);
        }

        template<typename F, typename... Args>
        decltype(auto) static_call_shared(F&& f, Args&&... args)
        {
            if constexpr (framework::shared_lockable<container_lock>)
                std::shared_lock<container_lock> guard(containerLock);
            else
                std::lock_guard<container_lock> guard(containerLock);
            return std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
        }

        // 独占调用
        template<typename F, typename... Args>
        decltype(auto) call_unique(F&& f, Args&&... args)
        {
            if constexpr (framework::shared_lockable<container_lock>)
                std::unique_lock<container_lock> guard(containerLock);
            else
                std::lock_guard<container_lock> guard(containerLock);
            return std::invoke(std::forward<F>(f), container, std::forward<Args>(args)...);
        }

        template<typename F, typename... Args>
        decltype(auto) static_call_unique(F&& f, Args&&... args)
        {
            if constexpr (framework::shared_lockable<container_lock>)
                std::unique_lock<container_lock> guard(containerLock);
            else
                std::lock_guard<container_lock> guard(containerLock);
            return std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
        }

        // 直接上锁
        auto lock() { return containerLock.lock(); }
        auto unlock() { return containerLock.unlock(); }
        auto try_lock() { return containerLock.try_lock(); }

        auto lock_shared()
        {
            if constexpr (framework::shared_lockable<container_lock>)
                return containerLock.lock_shared();
            else
                return containerLock.lock();
        }
        auto unlock_shared()
        {
            if constexpr (framework::shared_lockable<container_lock>)
                return containerLock.unlock_shared();
            else
                return containerLock.unlock();
        }
        auto try_lock_shared()
        {
            if constexpr (framework::shared_lockable<container_lock>)
                return containerLock.try_lock_shared();
            else
                return containerLock.try_lock();
        }
    };


}