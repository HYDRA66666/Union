#pragma once
#include "framework.h"
#include "pch.h"

#include "logger.h"

namespace HYDRA15::Union::referee
{
    // 提供错误保护、进度管理、守护线程的功能
    class iExceptionManager
    {
        static struct visualize
        {
            static_string warpmNote = "An error occurred while executing task: {}.";
            static_string warpnNote = "An error occurred while executing task {}: {}.";
            static_string warptNote = "After retrying {} times, task {} still failed: {}.";
        }vlsz;
    private:
        inline static secretary::logger lgr{ "iExceptionManager" };

        /***************************** 错误保护静态方法 *****************************/
        // 执行函数，如果出错，提供多种处理方法
        // 出错则打印错误信息
        template<typename F, typename ...Args>
        static auto warp_m(F f, Args... args) -> decltype(f(std::forward(args...)))
        {
            using Ret = decltype(f(std::forward(args...)));
            try { return f(std::forward(args...)); }
            catch (const std::exception& e) { lgr.error(vslz.warpmNote.data(), e.what());  return Ret(); }
        }
        // 出错则打印任务名称和信息
        template<typename F, typename ...Args>
        static auto warp_n(std::string taskName, F f, Args... args) -> decltype(f(std::forward(args...)))
        {
            using Ret = decltype(f(std::forward(args...)));
            try { return f(std::forward(args...)); }
            catch (const std::exception& e) { lgr.error(vslz.warpnNote.data(), taskName, e.what()); return Ret(); }
        }
        // 出错则重试指定次数，否则打印错误信息
        template<typename F, typename ...Args>
        static auto warp_t(std::string taskName, int times, F f, Args... args) -> decltype(f(std::forward(args...)))
        {
            using Ret = decltype(f(std::forward(args...)));
            int count = 0;
            while(count<times)
            {
                try { return f(std::forward(args...)); }
                catch (const std::exception& e) 
                { 
                    count++; 
                    if (count >= times) 
                    { 
                        lgr.error(vslz.warptNote.data(), times, taskName, e.what());
                        return Ret();
                    } 
                }
            }
        }
        // 出错则打印错误消息，并调用回调
        template<typename Ret, typename ...Args>
        static Ret warp_c(std::function<Ret(const std::exception&)> callback, std::function<Ret(Args...)> f, Args... args)
        {
            try { return f(std::forward(args...)); }
            catch (const std::exception& e) { lgr.error(vslz.warpmNote.data(), e.what());  if (callback)return callback(e); else return Ret(); }
        }
    };
}