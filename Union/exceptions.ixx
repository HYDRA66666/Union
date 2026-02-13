export module HYDRA15.Union.exceptions;

import std;
import HYDRA15.Union.astring;

namespace HYDRA15::Union 
{
    export class exception : public std::exception
    {
    public:
        exception(const astring& d)
            :desp(d)
        {
            auto stks = std::stacktrace::current();
            for (const auto& e : stks)
                stackTrace.push_back(format_stacktrace_entry(e));
        }
        virtual ~exception() = default;

    public:
        virtual const char* what() const noexcept override { return desp.string().c_str(); }

    public:
        const std::list<astring>& stack_trace() const { return stackTrace; }    // 返回调用栈列表

        std::string trace() const   // 返回格式化的调用栈字符串
        {
            if (stackTrace.empty())return {};
            size_t size = 13;
            for (const auto& i : stackTrace)
                size += i.string().size() + 6;
            std::string res{ "Stack trace:\n" };
            res.reserve(size);
            for (const auto& i : stackTrace)
                res.append(i.string().empty() ? "" : std::format("    {}\n", i.string()));
            res.pop_back(); res.pop_back();
            return res;
        }

    private:
        const astring desp;             // 错误描述
        std::list<astring> stackTrace;  // 调用栈

    private:    // 工具函数
        static astring format_stacktrace_entry(const std::stacktrace_entry& e)
        {
            if (e.source_file().empty())
                return astring(std::format("at {}", e.description()));

            std::filesystem::path path{ e.source_file() };
            auto fileName = path.filename();
            auto filePath = path.parent_path().filename();
            return astring(std::format(
                "at {} in {} ({}),",
                e.description(),
                fileName.string(),
                e.source_line()
            ));
        }
    };


    namespace exceptions
    {
        class interface_not_implemented : public exception
        {
        public:
            interface_not_implemented(const astring& interfaceCode)
                : exception(astring(std::format("The interface '{}' is not implemented", interfaceCode.string())))
            {
            }
        };


    }
}