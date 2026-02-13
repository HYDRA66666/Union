#pragma once
#include "framework.h"
#include "pch.h"

#include "lib_exceptions.h"

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
        datetime() :stamp(std::time(NULL)){}
        datetime(time_t t) :stamp(t){}
        datetime(const datetime&) = default;
        datetime& operator=(const datetime&) = default;
        ~datetime() = default;

        // 输出
    public:
        std::string date_time(
            std::string format = "%Y-%m-%d %H:%M:%S",
            int timeZone = localTimeZone
        ) const {
            if (timeZone < -12 || timeZone > 14)
                throw exceptions::common::BadParameter("timeZone", std::to_string(timeZone), "-12 < timeZone < 14");

            time_t localStamp = stamp + timeZone * 3600LL;
            tm local;
            gmtime_s(&local, &localStamp);
            std::string str;
            str.resize(format.size() * 2 + 20, '\0');
            size_t len = strftime(str.data(), str.size(), format.data(), &local);
            str.resize(len);
            return str;
        }

        // 静态工具函数
    public:
        static datetime now(){ return datetime(); }
        static std::string now_date_time(
            std::string format = "%Y-%m-%d %H:%M:%S", 
            int timeZone = localTimeZone
        ){
            return datetime().date_time(format, timeZone);
        }
    };
}