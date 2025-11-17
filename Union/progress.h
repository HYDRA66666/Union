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
        static std::string digital(std::string title, float progress)
        {
            if (progress < 0.0f) progress = 0.0f;
            if (progress > 1.0f) progress = 1.0f;
            return std::format(
                vslz.digitalProgressFormat.data(),
                title,
                static_cast<int>(progress * 100)
            );
        }

        static std::string simple_bar(std::string title, float progress, unsigned int barWidth = 10, char barChar = '=')
        {
            if (progress < 0.0f) progress = 0.0f;
            if (progress > 1.0f) progress = 1.0f;

            size_t bar = static_cast<size_t>(barWidth * progress);
            size_t space = barWidth - bar;

            return std::format(
                vslz.simpleBarFormat.data(),
                title,
                std::string(bar, barChar) + std::string(space, ' '),
                static_cast<int>(progress * 100)
            );
        }
    };

}