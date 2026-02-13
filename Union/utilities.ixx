export module HYDRA15.Union.utilities;

import std;

namespace HYDRA15::Union
{
    // 获取本地当前日期时间字符串（自动识别时区）
    // 默认格式：YYYY-MM-DD HH:MM:SS
    export inline std::string now_date_time()
    {
        using namespace std::chrono;

        const auto now = system_clock::now();
        const auto tz  = std::chrono::current_zone();
        const auto zt  = std::chrono::zoned_time<std::chrono::system_clock::duration>{ tz, now };

        // 使用 chrono::format 支持运行时 chrono 格式（例如 "%F %T"）
        return std::format("{:%F %T}", zt);
    }
}

