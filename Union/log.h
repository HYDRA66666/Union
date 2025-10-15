#pragma once
#include "framework.h"
#include "pch.h"

#include "secretary_exception.h"
#include "PrintCenter.h"

namespace HYDRA15::Union::secretary
{
    // 格式化日志字符串
    // 返回格式化后的字符串，用户需要自行处理输出
    class log
    {
        // 禁止构造
    private:
        log() = delete;
        log(const log&) = delete;
        ~log() = delete;

        // 私有数据
    private:
        static struct visualize
        {
            static_string info = "\033[0m[ {0} | INFO ][ {1} ] {2}\033[0m";
            static_string warn = "\033[0m[ {0} | \033[33mWARN\033[0m ][ {1} ] {2}\033[0m";
            static_string error = "\033[0m[ {0} |\033[35mERROR\033[0m ][ {1} ] {2}\033[0m";
            static_string fatal = "\033[0m[ {0} |\033[31mFATAL\033[0m ][ {1} ] \033[31m{2}\033[0m";
            static_string debug = "\033[0m[ {0} |\033[2mDEBUG\033[0m ][ {1} ] {2}\033[0m";
            static_string trace = "\033[0m[ {0} |\033[34mTRACE\033[0m ][ {1} ] {2}\033[0m";
        }vslz;

        
        
        // 公有接口
    public:
        static std::string info(const std::string& title, const std::string& content);
        static std::string warn(const std::string& title, const std::string& content);
        static std::string error(const std::string& title, const std::string& content);
        static std::string fatal(const std::string& title, const std::string& content);
        static std::string debug(const std::string& title, const std::string& content);
        static std::string trace(const std::string& title, const std::string& content);

        // 配置项
    public:
        inline static std::function<void(const std::string&)> print = [](const std::string& str) {PrintCenter::get_instance() << str; };
        static inline bool enableDebug = debug;
    };
}