export module HYDRA15.Union.log;

import std;
import HYDRA15.Union.astring;
import HYDRA15.Union.utilities;

namespace HYDRA15::Union
{
    namespace log
    {
        static struct visualize
        {
            static constexpr std::string_view info{ "[ {0} | INFO ] [ {1} ] {2}" };
            static constexpr std::string_view warn{ "[ {0} | WARN ] [ {1} ] {2}" };
            static constexpr std::string_view error{ "[ {0} | ERROR ][ {1} ] {2}" };
        }vslz;

        static struct visualize_color
        {
            static constexpr std::string_view info{ "\033[0m[ {0} | INFO ] [ {1} ] {2}\033[0m" };
            static constexpr std::string_view warn{ "\033[0m[ {0} | \033[33mWARN\033[0m ] [ {1} ] {2}\033[0m" };
            static constexpr std::string_view error{ "\033[0m[ {0} | \033[35mERROR\033[0m ][ {1} ] {2}\033[0m" };
        }vslzclr;

        //配置项
        export std::function<void(const std::string&)> print = [](const std::string& str) { std::cout << str + "\n"; };
        export bool colourful = true;

        // 日志接口
        export std::string info(const std::string& title, const std::string& content)
        {
            std::string str;
            std::string date = now_date_time();
            if (colourful)str = std::format(vslzclr.info, date, title, content);
            else str = std::format(vslz.info, date, title, content);
            if (print)print(str);
            return str;
        }

        export std::string warn(const std::string& title, const std::string& content)
        {
            std::string str;
            std::string date = now_date_time();
            if (colourful)str = std::format(vslzclr.warn, date, title, content);
            else str = std::format(vslz.warn, date, title, content);
            if (print)print(str);
            return str;
        }

        export std::string error(const std::string& title, const std::string& content)
        {
            std::string str;
            std::string date = now_date_time();
            if (colourful)str = std::format(vslzclr.error, date, title, content);
            else str = std::format(vslz.error, date, title, content);
            if (print)print(str);
            return str;
        }
    }

    export class logger
    {
    public:
        logger();
        logger(const std::string& name) : name(name) {}

        operator bool() const { return !name.empty(); }

        template<typename... Args>
        std::string info(const std::string& fstr, Args... args) { return log::info(name, std::vformat(fstr, std::make_format_args(args...))); }

        template<typename... Args>
        std::string warn(const std::string& fstr, Args... args) { return log::warn(name, std::vformat(fstr, std::make_format_args(args...))); }

        template<typename... Args>
        std::string error(const std::string& fstr, Args... args) { return log::error(name, std::vformat(fstr, std::make_format_args(args...))); }
    private:
        std::string name;
    };
}