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


    class files : public referee::iExceptionBase
    {
        static constexpr expt_code cateExptCode = 0x000A0000;
        
    public:
        files& set(const std::string& k, const std::string& v) { iExceptionBase::set(k, v); return *this; }

    public:
        files(const std::filesystem::path& path, expt_code code, const std::string& desp)
            : iExceptionBase(cateExptCode | code, desp) {
            set("file path", path.string());
        }

        virtual ~files() = default;

    public:
        static files FileIOFlowError(const std::filesystem::path& path, int state) noexcept
        {
            return files{ path, 0xA01, "File stream flow error" }
            .set("state", std::to_string(state));
        }

        static files FileNotExist(const std::filesystem::path& path) noexcept
        {
            return files{ path,0xA02,"File not exist" };
        }

        static files FileAreadyExist(const std::filesystem::path& path) noexcept
        {
            return files{ path,0xA03,"File aready exist" };
        }

        static files FileFull(const std::filesystem::path& path, size_t limit) noexcept
        {
            return files{ path,0xA04,"File size has reached the configured limit." }.set("limit", std::to_string(limit));
        }

        static files ExceedRage(const std::filesystem::path& path, const std::string& relParam, const std::string& req) noexcept
        {
            return files{ path,0xB01,"File request exceed allowed range" }
            .set("relative parameter", relParam).set("reason", req);
        }

        static files FormatNotSupported(const std::filesystem::path& path, const std::string& req) noexcept
        {
            return files{ path,0xB02,"The file format is not supported" }
            .set("reason", req);
        }

        static files ContentNotFound(const std::filesystem::path& path) noexcept
        {
            return files{ path,0xB03,"The requested content was not found." };
        }

        
    };
}