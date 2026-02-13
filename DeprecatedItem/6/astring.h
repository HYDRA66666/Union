#pragma once
#include <string>

namespace HYDRA15::Union::framework
{
    // 仅接受 ascii 字符的字符串
    // 继承自 string_view ，仅可在编译器初始化，不可修改
    class astring : public std::string_view
    {
    public:
        constexpr astring() = default;
        constexpr astring(const astring&) = default;
        constexpr astring(const std::string_view& s) :std::string_view(s) { verify(); }
        constexpr astring(const char* p, size_t s) : std::string_view(p, s) { verify(); }
        constexpr astring(const char* p) : std::string_view(p) { verify(); }

        template<size_t N>
        constexpr astring(const char(&a)[N])
            :std::string_view(a, N)
        {
            verify();
        }


        constexpr ~astring() = default;

        constexpr static bool is_ascii(const char& c){ return (c >= 0 && c <= 127) ? true : false; }
        constexpr void verify()
        {
            for (const auto& c : *this)
                if (!is_ascii(c))
                    throw std::exception("astring can only handle ascii characters");
        }
    };
}
