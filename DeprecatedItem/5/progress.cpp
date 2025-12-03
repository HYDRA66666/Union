#include "pch.h"
#include "progress.h"

namespace HYDRA15::Union::secretary
{
    std::string progress::digital(std::string title, float progress)
    {
        if (progress < 0.0f) progress = 0.0f;
        if (progress > 1.0f) progress = 1.0f;
        return std::format(
            vslz.digitalProgressFormat.data(),
            title,
            static_cast<int>(progress * 100)
        );
    }

    std::string progress::simple_bar(std::string title, float progress, unsigned int barWidth, char barChar)
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
}
