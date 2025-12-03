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
            static_uint PropretiesParseFaild = 0x003;
            static_uint LocalByteOrderUncertain = 0x004;

            // files
            static_uint files = 0xA00;
            static_uint FileIOError = 0xA01;
            static_uint FileNotAccessable = 0xA02;
            static_uint FileInvalidAccess = 0xA03;
        }iExptCodes;

    private:
        static struct vslz
        {
            static_string assistant = "Unknown Assistant Exception";
            static_string DateTimeInvalidTimeZone = "DateTime: Invalid Time Zone";
            static_string UtilityInvalidChar = "Invalid character detected";
            static_string PropretiesParseFaild = "Faild to parse propreties.";
            static_string LocalByteOrderUncertain = "Local byte order is uncertain";

            static_string FileIOError = "Error occures during file io operation";
            static_string FileIOErrorFormat = "Error occures during file io operation: {}:{}";
            static_string FileNotAccessable = "File not accessable";
            static_string FileNotAccessableFormat = "File not accessable: {}:{}";
            static_string FileInvalidAccess = "The file access request is invalid";
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
        static assistant PropretiesParseFaild() noexcept;
        static assistant LocalByteOrderUncertain() noexcept;

        static assistant FileIOError(const std::error_code& ec) noexcept;
        static assistant FileIOError() noexcept;
        static assistant FileNotAccessable(const std::error_code& ec) noexcept;
        static assistant FileNotAccessable() noexcept;
        static assistant FileInvalidAccess() noexcept;
    };
}