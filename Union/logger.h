#pragma once
#include "framework.h"
#include "pch.h"

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
        logger(const std::string& title) :title(title) {}

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
#define UNION_CREATE_LOGGER() HYDRA15::Union::secretary::logger{__func__}
#endif // !UNION_CREATE_LOGGER

}
