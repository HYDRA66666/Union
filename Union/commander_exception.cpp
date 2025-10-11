#include "pch.h"
#include "commander_exception.h"

namespace HYDRA15::Union::exceptions
{
    commander::commander(const std::string& desp, const referee::iException_code& code) noexcept
        :referee::iExceptionBase(desp, framework::libID.commander, code)
    {
    }

    commander commander::make_exception(const referee::iException_code& code) noexcept
    {
        return commander(vslz.commander.data(), code);
    }

    // 快速创建异常的函数模板
#define make(type)                                              \
    commander commander::type() noexcept                        \
    {                                                           \
        return commander(vslz.type.data(),iExptCodes.type);     \
    }

    make(CommandUnknownExpt);
    make(CommandAsyncInputNotAllowed);

#undef make

    commander commander::NoSuchCommand(const std::string& cmdline) noexcept
    {
        return commander(std::format(vslz.CommandNoSuchCommand.data(), cmdline), iExptCodes.CommandNoSuchCommand);
    }

    commander commander::EntrustNotInitFaild(int code) noexcept
    {
        return commander(std::format(vslz.EntrustNotInitFaild.data(), code), iExptCodes.EntrustNotInitFaild);
    }

}