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

            // DateTime
            static_uint dateTime = 0xA00;
            static_uint dateTimeInvalidTimeZone = 0xA01;

            // utility
            static_uint utility = 0xB00;
            static_uint utilityInvalidChar = 0xB01;
        }iExptCodes;

    private:
        static struct vslz
        {
            static_string assistant = "Unknown Assistant Exception";

            // DateTime
            static_string dateTime = "Unknown DateTime Exception";
            static_string dateTimeInvalidTimeZone = "DateTime: Invalid Time Zone";

            //utility
            static_string utility = "Unknown utility exception";
            static_string utilityInvalidChar = "Invalid character detected";
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

        // DateTime
        static assistant DateTimeUnknownException();
        static assistant DateTimeInvalidTimeZone();

        // utility
        static assistant UtilityUnknownException();
        static assistant UtilityInvalidChar();
    };
}