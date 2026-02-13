export module HYDRA15.Union.astring;

import std;

namespace HYDRA15::Union
{
    // 仅接受 ascii 字符的字符串
    export class astring
    {
    public:
        constexpr astring() = default;
        constexpr astring(const astring&) = default;
        constexpr astring(const std::string_view& s) : data(s) { verify(); }
        constexpr astring(const char* p, size_t s) : data(p, s) { verify(); }
        constexpr astring(const char* p) : data(p) { verify(); }
        template<size_t N> constexpr astring(const char(&a)[N]) : data(a, N) { verify(); }
        constexpr ~astring() = default;

        const std::string& string() const { return data; }
        std::string string() { return data; }
        operator std::string() const { return data; }
        std::string_view view() const { return std::string_view(data); }

    private:
        const std::string data;

        constexpr void verify()
        {
            constexpr auto is_ascii = [](char c) { return static_cast<unsigned char>(c) <= 127; };
            for (const auto& c : data)
                if (!is_ascii(c))
                    throw std::exception("astring can only handle ascii characters");
        }
    };

}