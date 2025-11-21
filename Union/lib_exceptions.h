#pragma once
#include "pch.h"
#include "framework.h"

#include "iExceptionBase.h"
#include "lib_info.h"

namespace HYDRA15::Union::exceptions
{
    // 用于没有其他特殊处理的错误
    class common : public referee::iExceptionBase
    {
        static constexpr expt_code cateExptCode = 0x00000000;

    public:
        common& set(const std::string& k, const std::string& v) { iExceptionBase::set(k, v); return *this; }

        common(expt_code libID, expt_code e, const std::string& desp)
            :iExceptionBase(cateExptCode | libID | e, desp) {
        }
        common(const std::string& desp) :iExceptionBase(cateExptCode, desp) {}
        virtual ~common() = default;

    public: // 快速创建常见异常
        // 杂项
        static common BadParameter(const std::string& param, const std::string& provided, const std::string& desired) noexcept
        {
            return common{ cateExptCode, 0x001, "The provided argument is invalid" }
            .set("param", param).set("provided", provided).set("desired", desired);
        }

        static common UnsupportedFormat(const std::string& desp) noexcept
        {
            return common{ cateExptCode,0x002,"Target object contains unsupported format" }
            .set("requires", desp);
        }
    };


    class fstream : public referee::iExceptionBase
    {
        static constexpr expt_code cateExptCode = 0x000A0000;
        const std::filesystem::path path;
        
    public:
        fstream& set(const std::string& k, const std::string& v) { iExceptionBase::set(k, v); return *this; }

    public:
        fstream(const std::filesystem::path& path, expt_code code, const std::string& desp)
            : iExceptionBase(cateExptCode | code, std::format("An error occurred while operating on file {}: {}", path.string(), desp)) {
            set("path", path.string());
        }

        virtual ~fstream() = default;

    public:
        static fstream FileIOFlowError(const std::filesystem::path& path, int state) noexcept
        {
            return fstream{ path, 0x01, "fstream error" }
            .set("state", std::to_string(state));
        }

        static fstream ExceedRage(const std::filesystem::path& path, const std::string& relParam, const std::string& req)
        {
            return fstream{ path,0x02,"Data size exceeds the allowed limit" }
            .set("relative parameter", relParam).set("requirements", req);
        }
    };
}