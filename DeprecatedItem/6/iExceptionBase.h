#pragma once
#include "pch.h"
#include "framework.h"

namespace HYDRA15::Union::referee
{
    class iExceptionBase : public std::exception
    {
    public:
        using expt_code = unsigned int;

    private:    // 数据
        const expt_code exptCode;           // 错误码（必需）
        const std::string desp;             // 错误描述
        std::unordered_map<std::string, std::string> info;  // 错误信息 / 参数
        std::list<std::string> stackTrace;  // 调用栈
    public:     // 数据配置
        static inline bool enableDebug = debug;

    private:    // 工具函数
#ifdef UNION_IEXPT_STACKTRACE_ENABLE
        static std::string format_stacktrace_entry(const std::stacktrace_entry& e)
        {
            if (e.source_file().empty())
                return std::format("at {}", e.description());

            std::filesystem::path path{ e.source_file() };
            auto fileName = path.filename();
            auto filePath = path.parent_path().filename();
            return std::format(
                "at {} in {} ({}),",
                e.description(),
                std::format("{}/{}",filePath.string(), fileName.string()),
                e.source_line()
            );
        }
#endif

    public:     // 接口
        expt_code code() const { return exptCode; }

        const std::string& description() const { return desp; }

        const std::unordered_map<std::string, std::string>& infomation() const { return info; }
        const std::string& get(const std::string& k)const { return info.at(k); }
        iExceptionBase& set(const std::string& k, const std::string& v) { info[k] = v; return *this; }

        const std::list<std::string>& stack_trace() const { return stackTrace; }

    protected:
        mutable std::string whatStr;
    public:     // 系统接口
        virtual const char* what() const noexcept override
        {
            if (desp.empty())
                whatStr = std::format("unknow exception. (0x{:08X})", exptCode);
            else
                whatStr = std::format("{}. (0x{:08X})", desp, exptCode);
            if (enableDebug)
            {
                if (!info.empty())whatStr += " " + detail();
                if (!stackTrace.empty())whatStr += "\n" + trace();
            }
            return whatStr.c_str();
        }

    public:     // 扩展信息接口
        virtual std::string detail() const
        {
            if (info.empty())return {};
            size_t size = 2;
            for (const auto& [k, v] : info)
                size += k.size() + v.size() + 5;
            std::string res{ "[" };
            res.reserve(size);
            for (const auto& [k, v] : info)
                res.append(std::format("{} : {}, ", k, v));
            res.pop_back(); res.pop_back();
            res.append("]");
            return res;
        }

        virtual std::string trace() const
        {
            if (stackTrace.empty())return {};
            size_t size = 13;
            for (const auto& i : stackTrace)
                size += i.size() + 6;
            std::string res{ "Stack trace:\n" };
            res.reserve(size);
            for (const auto& i : stackTrace)
                res.append(i.empty() ? "" : std::format("    {}\n", i));
            res.pop_back(); res.pop_back();
            return res;
        }

    public:     // 构造
        iExceptionBase(expt_code c, const std::string& d = std::string{}) 
            :exptCode(c), desp(d)
        {
#ifdef UNION_IEXPT_STACKTRACE_ENABLE
            auto stks = std::stacktrace::current();
            size_t i = 0;
            for (const auto& e : stks)
                stackTrace.push_back(format_stacktrace_entry(e));
#endif // UNION_IEXPT_STACKTRACE_ENABLE
        }

        virtual ~iExceptionBase() = default;
    };


}