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

    // 快速创建异常的函数模板
#define make(type)                                              \
    assistant assistant::type() noexcept                        \
    {                                                           \
        return assistant(vslz.type.data(),iExptCodes.type);     \
    }

    make(DateTimeInvalidTimeZone);
    make(UtilityInvalidChar);

#undef make



}