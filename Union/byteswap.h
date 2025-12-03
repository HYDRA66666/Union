#pragma once
#include "pch.h"
#include "framework.h"

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
        else throw exceptions::common("Local byte order uncertain");
    }

    template<typename I>
        requires std::is_integral_v<I>
    I to_big_endian(I i)
    {
        if constexpr (std::endian::native == std::endian::little)
            return std::byteswap(i);
        else if constexpr (std::endian::native == std::endian::big)
            return i;
        else throw exceptions::common("Local byte order uncertain");
    }

    template<typename I>
        requires std::is_integral_v<I>
    I from_little_endian(I i)
    {
        if constexpr (std::endian::native == std::endian::little)
            return i;
        else if constexpr (std::endian::native == std::endian::big)
            return std::byteswap(i);
        else throw exceptions::common("Local byte order uncertain");
    }

    template<typename I>
        requires std::is_integral_v<I>
    I from_big_endian(I i)
    {
        if constexpr (std::endian::native == std::endian::little)
            return std::byteswap(i);
        else if constexpr (std::endian::native == std::endian::big)
            return i;
        else throw exceptions::common("Local byte order uncertain");
    }



    // 整组字节转换，直接在传入的数据中原地转换
    template<typename T>
        requires std::is_integral_v<T>
    void to_little_endian_vector(std::vector<T>& data)
    {
        for (auto& i : data)
            i = to_little_endian<T>(i);
    }

    template<typename T>
        requires std::is_integral_v<T>
    void to_big_endian_vector(std::vector<T>& data)
    {
        for (auto& i : data)
            i = to_big_endian<T>(i);
    }

    template<typename T>
        requires std::is_integral_v<T>
    void from_little_endian_vector(std::vector<T>& data)
    {
        for (auto& i : data)
            i = from_little_endian<T>(i);
    }

    template<typename T>
        requires std::is_integral_v<T>
    void from_big_endian_vector(std::vector<T>& data)
    {
        for (auto& i : data)
            i = from_big_endian<T>(i);
    }

    // Range 版本：支持任意可迭代容器（传入可写容器引用）
    // ** AI 生成的代码，未经广泛测试
    template<typename R>
        requires std::is_integral_v<std::remove_reference_t<decltype(*std::begin(std::declval<R&>()))>>
    void to_little_endian_range(R& data)
    {
        for (auto& i : data)
            i = to_little_endian(std::remove_reference_t<decltype(i)>(i));
    }

    template<typename R>
        requires std::is_integral_v<std::remove_reference_t<decltype(*std::begin(std::declval<R&>()))>>
    void to_big_endian_range(R& data)
    {
        for (auto& i : data)
            i = to_big_endian(std::remove_reference_t<decltype(i)>(i));
    }

    template<typename R>
        requires std::is_integral_v<std::remove_reference_t<decltype(*std::begin(std::declval<R&>()))>>
    void from_little_endian_range(R& data)
    {
        for (auto& i : data)
            i = from_little_endian(std::remove_reference_t<decltype(i)>(i));
    }

    template<typename R>
        requires std::is_integral_v<std::remove_reference_t<decltype(*std::begin(std::declval<R&>()))>>
    void from_big_endian_range(R& data)
    {
        for (auto& i : data)
            i = from_big_endian(std::remove_reference_t<decltype(i)>(i));
    }
}