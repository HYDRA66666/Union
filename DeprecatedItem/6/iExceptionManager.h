#pragma once
#include "framework.h"
#include "pch.h"

namespace HYDRA15::Union::referee
{
    // 提供错误处理相关工具
    class iExceptionManager
    {
        static struct visualize
        {

        }vslz;

        // 程序包装器，简单保护程序不因为异常而崩溃
    public:
        // 简单包装，仅用于保证程序不崩溃
        template<typename F, typename... Args>
        static auto warp(F f, Args... args)
            -> std::expected<std::invoke_result_t<F, Args...>, std::exception_ptr>
        {
            try { return std::invoke(std::forward<F>(f), std::forward<Args>(args)...); }
            catch (...) { return std::unexpected(std::current_exception()); }
        }
        // 带出错回调的包装
        template<typename F, typename... Args>
        static auto warp_c(std::function<void(std::exception_ptr)> callback, F f, Args... args)
            -> std::expected<std::invoke_result_t<F, Args...>, std::exception_ptr>
        {
            try { return std::invoke(std::forward<F>(f), std::forward<Args>(args)...); }
            catch (...) { callback(std::current_exception); return std::unexpected(std::current_exception()); }
        }
        // 带重试的包装
        template<typename F, typename... Args>
        static auto warp_r(unsigned int times, F f, Args... args)
            -> std::expected<std::invoke_result_t<F, Args...>, std::exception_ptr>
        {
            unsigned int i = 0;
            while (true)
            {
                try { return std::invoke(std::forward<F>(f), std::forward<Args>(args)...); }
                catch (...) { if (i < times) { i++; continue; } else return std::unexpected(std::current_exception()); }
            }
        }
        // 带重试和回调的包装
        template<typename F, typename... Args>
        static auto warp_rc(unsigned int times, std::function<void(std::exception_ptr)> callback, F f, Args... args)
            -> std::expected<std::invoke_result_t<F, Args...>, std::exception_ptr>
        {
            unsigned int i = 0;
            while (true)
            {
                try { return std::invoke(std::forward<F>(f), std::forward<Args>(args)...); }
                catch (...) 
                { 
                    if (i < times) { i++; continue; } 
                    else { callback(std::current_exception); return std::unexpected(std::current_exception()); }
                }
            }
        }
    };

}