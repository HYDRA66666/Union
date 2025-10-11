#pragma once
#include "framework.h"
#include "pch.h"

#include "iExceptionBase.h"
#include "libID.h"

namespace HYDRA15::Union::exceptions
{
    class assistant : public referee::iExceptionBase
    {
    public:
        static struct iException_codes
        {
            static_uint assistant = 0x0000;
            static_uint DateTimeInvalidTimeZone = 0x001;
            static_uint UtilityInvalidChar = 0x002;
        }iExptCodes;

    private:
        static struct vslz
        {
            static_string assistant = "Unknown Assistant Exception";
            static_string DateTimeInvalidTimeZone = "DateTime: Invalid Time Zone";
            static_string UtilityInvalidChar = "Invalid character detected";
        }vslz;

    public:
        assistant(
            const std::string& desp = vslz.assistant.data(),
            const referee::iException_code& code = iExptCodes.assistant
        ) noexcept;
        assistant() = delete;
        virtual ~assistant() noexcept = default;

        // 快速创建异常
        static assistant make_exception(const referee::iException_code& exptCode = iExptCodes.assistant) noexcept;

        static assistant DateTimeInvalidTimeZone() noexcept;
        static assistant UtilityInvalidChar() noexcept;
    };
}