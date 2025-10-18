#include "pch.h"
#include "log.h"

namespace HYDRA15::Union::secretary
{
    // 输出日志的函数模板
#define logging(Type)                                                               \
    std::string log::Type(const std::string& title, const std::string& content)     \
    {                                                                               \
        std::string str = std::format(                                              \
            vslz.Type.data(),                                                       \
            assistant::datetime::now_date_time("%Y-%m-%d %H:%M:%S"),                \
            title,                                                                  \
            content                                                                 \
        );                                                                          \
        if(print)                                                                   \
            print(str);                                                             \
        return str;                                                                 \
    }

    logging(info);
    logging(warn);
    logging(error);
    logging(fatal);

#undef logging

    std::string log::debug(const std::string& title, const std::string& content)
    {
        std::string str = std::format(
            vslz.debug.data(),
            assistant::datetime::now_date_time("%Y-%m-%d %H:%M:%S"),
            title,
            content
        );
        if (enableDebug && print)
            print(str);
        return str;
    }

    std::string log::trace(const std::string& title, const std::string& content)
    {
        std::string str = std::format(
            vslz.trace.data(),
            assistant::datetime::now_date_time("%Y-%m-%d %H:%M:%S"),
            title,
            content
        );
        if (enableDebug && print)
            print(str);
        return str;
    }
    
}
