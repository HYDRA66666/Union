#pragma once
#include "framework.h"
#include "pch.h"

#include "iExceptionBase.h"
#include "libID.h"

namespace HYDRA15::Union::exceptions
{
    class labourer : public referee::iExceptionBase
    {
    public:
        static struct iException_codes
        {
            // 线程池
            static_uint threadLake = 0xA00;
            static_uint threadQueueFull = 0xA01;
            static_uint emptyTask = 0xA02;
        }iExptCodes;

    private:
        static struct visualize
        {
            // 线程池
            static_string thrdLakeExpt = "ThreadLake Exception";
            static_string thrdLakeQueueFull = "ThreadLake: Task queue is full";
            static_string thrdLakeEmptyTask = "ThreadLake: Empty task";
        }vslz;

    public:
        labourer(
            const std::string& desp = vslz.thrdLakeExpt.data(),
            const referee::iException_code& code = iExptCodes.threadLake
        ) noexcept;
        labourer() = delete;
        virtual ~labourer() noexcept = default;


        // 快速创建异常
        static labourer make_exception(const referee::iException_code& exptCode) noexcept;

        static labourer TaskQueueFull() noexcept;
        static labourer EmptyTask() noexcept;

    };
}