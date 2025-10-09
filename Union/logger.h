#pragma once
#include "framework.h"
#include "pch.h"

#include "secretary_exception.h"
#include "log.h"

namespace HYDRA15::Union::secretary
{
    // 日志输出代理
    class logger
    {
        const std::string title;

    public:
        logger() = delete;
        logger(const logger&) = default;
        logger(const std::string&);

        std::string info(const std::string& content);
        std::string warn(const std::string& content);
        std::string error(const std::string& content);
        std::string fatal(const std::string& content);
        std::string debug(const std::string& content);
        std::string trace(const std::string& content);

#define logf(type) template<typename ... Args> std::string type(const std::string& fstr, Args...args) { return log::type(title, std::vformat(fstr, std::make_format_args(args...))); }

        logf(info);
        logf(warn);
        logf(error);
        logf(fatal);
        logf(debug);
        logf(trace);

#undef logf
    };

#ifndef UNION_CREATE_LOGGER
#define UNION_CREATE_LOGGER() logger{__func__}
#endif // !UNION_CREATE_LOGGER

}
