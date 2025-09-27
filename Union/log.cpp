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
        lib_default_print(str);                                                     \
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
        if(enableDebug)
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
        if (enableDebug)
            print(str);
        return str;
    }

    void log::debug(bool d)
    {
        log::enableDebug = d;
    }

    void log::set_print(std::function<void(const std::string&)> f)
    {
        print = f;
    }
    
    
    //std::string log::info(const std::string& title, const std::string& content)
    //{
    //    std::string str = std::format(
    //        vslz.logFormatClr.data(),
    //        assistant::datetime::now_date_time("%Y-%m-%d %H:%M:%S"),
    //        title,
    //        content
    //    );
    //    lib_default_print(str);
    //    return str;
    //}

    //std::string log::warn(const std::string& title, const std::string& content)
    //{
    //    std::string str = std::format(
    //        vslz.warningFormatClr.data(),
    //        assistant::datetime::now_date_time("%Y-%m-%d %H:%M:%S"),
    //        title,
    //        content
    //    );
    //    lib_default_print(str);
    //    return str;
    //}

    //std::string log::error(const std::string& title, const std::string& content)
    //{
    //    std::string str = std::format(
    //        vslz.errorFormatClr.data(),
    //        assistant::datetime::now_date_time("%Y-%m-%d %H:%M:%S"),
    //        title,
    //        content
    //    );
    //    lib_default_print(str);
    //    return str;
    //}

    //std::string log::fatal(const std::string& title, const std::string& content)
    //{
    //    std::string str = std::format(
    //        vslz.fatalFormatClr.data(),
    //        assistant::datetime::now_date_time("%Y-%m-%d %H:%M:%S"),
    //        title,
    //        content
    //    );
    //    lib_default_print(str);
    //    return str;
    //}

    //std::string log::debug(const std::string& title, const std::string& content)
    //{
    //    std::string str = std::format(
    //        vslz.debugFormatClr.data(),
    //        assistant::datetime::now_date_time("%Y-%m-%d %H:%M:%S"),
    //        title,
    //        content
    //    );
    //    lib_default_print(str);
    //    return str;
    //}

    //std::string log::trace(const std::string& title, const std::string& content)
    //{
    //    std::string str = std::format(
    //        vslz.traceFormatClr.data(),
    //        assistant::datetime::now_date_time("%Y-%m-%d %H:%M:%S"),
    //        title,
    //        content
    //    );
    //    lib_default_print(str);
    //    return str;
    //}
}
