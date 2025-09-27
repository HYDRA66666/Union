#pragma once
#include "framework.h"
#include "pch.h"

namespace HYDRA15::Union::secretary
{
    // 格式化进度条
    // 返回格式化后的字符串，用户需要自行处理输出
    class progress
    {
        // 禁止构造
    private:
        progress() = delete;
        progress(const progress&) = delete;
        ~progress() = delete;

        // 公有数据
    private:
        static struct visualize
        {
            static_string digitalProgressFormat = "{0}... {1:02d}%";
            static_string simpleBarFormat = "{0}[{1}] {2:02d}%";
        }vslz;

        // 接口
    public:
        static std::string digital(std::string title, float progress);
        static std::string simple_bar(std::string title, float progress, unsigned int barWidth = 10, char barChar = '=');
    };

}