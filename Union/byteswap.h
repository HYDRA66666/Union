#pragma once
#include "pch.h"
#include "framework.h"

#include "assistant_exception.h"

namespace HYDRA15::Union::assistant::byteswap
{
    // 字节序转换
    template<typename I>
        requires std::is_integral_v<I>
    I to_little_endian(I i)
    {
        if constexpr (std::endian::native == std::endian::little)
            return i;
        else if constexpr (std::endian::native == std::endian::big)
            return std::byteswap(i);
        else throw exceptions::assistant::LocalByteOrderUncertain();
    }

    template<typename I>
        requires std::is_integral_v<I>
    I to_big_endian(I i)
    {
        if constexpr (std::endian::native == std::endian::little)
            return std::byteswap(i);
        else if constexpr (std::endian::native == std::endian::big)
            return i;
        else throw exceptions::assistant::LocalByteOrderUncertain();
    }

    template<typename I>
        requires std::is_integral_v<I>
    I from_little_endian(I i)
    {
        if constexpr (std::endian::native == std::endian::little)
            return i;
        else if constexpr (std::endian::native == std::endian::big)
            return std::byteswap(i);
        else throw exceptions::assistant::LocalByteOrderUncertain();
    }

    template<typename I>
        requires std::is_integral_v<I>
    I from_big_endian(I i)
    {
        if constexpr (std::endian::native == std::endian::little)
            return std::byteswap(i);
        else if constexpr (std::endian::native == std::endian::big)
            return i;
        else throw exceptions::assistant::LocalByteOrderUncertain();
    }
}