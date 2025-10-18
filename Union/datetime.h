#pragma once
#include "framework.h"
#include "pch.h"

#include "assistant_exception.h"

namespace HYDRA15::Union::assistant
{
    // 存储时间、提供时间和日期的格式化输出
    // 仅用于日期和时间的输出，不用于精确计时
    constexpr int localTimeZone = 8; // 本地时区，默认为东八区
    class datetime
    {
        // 记录的时间
        time_t stamp;

        // 构造和析构
    public:
        datetime();
        datetime(time_t t);
        datetime(const datetime&) = default;
        datetime& operator=(const datetime&) = default;
        ~datetime() = default;

        // 输出
    public:
        std::string date_time(std::string format = "%Y-%m-%d %H:%M:%S", int timeZone = localTimeZone) const;

        // 静态工具函数
    public:
        static datetime now();
        static std::string now_date_time(std::string format = "%Y-%m-%d %H:%M:%S", int timeZone = localTimeZone);
    };
}