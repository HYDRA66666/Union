#include "pch.h"
#include "secretary_exception.h"

namespace HYDRA15::Union::exceptions
{
    secretary::secretary(const std::string& desp, const referee::iException_code& code) noexcept
        : referee::iExceptionBase(desp, Union::framework::libID.secretary, code)
    {
    }

    secretary secretary::make_exception(const referee::iException_code& exptCode) noexcept
    {
        return secretary(
            vslz.secretary.data(),
            exptCode
        );
    }

    secretary secretary::PrintCenterUnknown() noexcept
    {
        return secretary(
            vslz.printCenter.data(),
            iExptCodes.printCenter
        );
    }

    secretary secretary::PrintCenterBtmMsgFull() noexcept
    {
        return secretary(
            vslz.printBtmMsgFull.data(),
            iExptCodes.printBtmMsgFull
        );
    }

    secretary secretary::PrintCenterBtmMsgNotFound() noexcept
    {
        return secretary(
            vslz.printBtmMsgNotFound.data(),
            iExptCodes.printBtmMsgNotFound
        );
    }


}