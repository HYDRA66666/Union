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
        static common BadParameter(const std::string& param, const std::string& provided, const std::string& desired)
        {
            common e{ cateExptCode, 0x001, "The provided argument is invalid" };
            e.set("param", param).set("provided", provided).set("desired", desired);
            return e;
        }

        static common UnsupportedFormat(const std::string& desp)
        {
            common e{ cateExptCode,0x002,"Target object contains unsupported format" };
            e.set("requires", desp);
            return e;
        }
    };
}