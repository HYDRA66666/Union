#include "pch.h"
#include "assistant_exception.h"

namespace HYDRA15::Union::exceptions
{
    assistant::assistant(const std::string& desp, const referee::iException_code& code) noexcept
        :referee::iExceptionBase(desp, framework::libID.assistant, code)
    {
    }

    assistant assistant::make_exception(const referee::iException_code& code) noexcept
    {
        return assistant(vslz.assistant.data(), code);
    }

    assistant assistant::DateTimeUnknownException()
    {
        return assistant(vslz.dateTime.data(), iExptCodes.dateTime);
    }

    assistant assistant::DateTimeInvalidTimeZone()
    {
        return assistant(vslz.dateTimeInvalidTimeZone.data(), iExptCodes.dateTimeInvalidTimeZone);
    }

    assistant assistant::UtilityUnknownException()
    {
        return assistant(vslz.utility.data(), iExptCodes.utility);
    }

    assistant assistant::UtilityInvalidChar()
    {
        return assistant(vslz.utilityInvalidChar.data(), iExptCodes.utilityInvalidChar);
    }




}