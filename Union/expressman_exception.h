#pragma once
#include "framework.h"
#include "pch.h"

#include "iExceptionBase.h"
#include "libID.h"

namespace HYDRA15::Union::exceptions
{
    class expressman : public referee::iExceptionBase
    {
    public:
        static struct iException_codes
        {
            static_uint expressman = 0x0000;

            // Interface
            static_uint InterfaceUnknownException = 0xA00;

            // mail
            static_uint BasicMailUnknownException = 0xB00;
            static_uint BasicMailEmptyCollector = 0xB01;
            static_uint BasicMailRequirementNotMet = 0xB02;
        }iExptCodes;

    private:
        static struct vslz
        {
            static_string expressman = "Unknown Expressman Exception";

            // Interface
            static_string InterfaceUnknownException = "Unknown expressman interface exception";

            // mail
            static_string BasicMailUnknownException = "Unknown expressman basic mail exception";
            static_string BasicMailEmptyCollector = "The specified collector does not exist";
            static_string BasicMailRequirementNotMet = "The requirement is not satisfied";
        }vslz;

    public:
        expressman(
            const std::string& desp = vslz.expressman.data(),
            const referee::iException_code& code = iExptCodes.expressman
        ) noexcept;
        expressman() = delete;
        virtual ~expressman() noexcept = default;

        // 快速创建异常
        static expressman make_exception(const referee::iException_code& exptCode = iExptCodes.expressman) noexcept;

        // Interface
        static expressman InterfaceUnknownException() noexcept;

        // basic mail
        static expressman BasicMailUnknownException() noexcept;
        static expressman BasicMailEmptyCollector() noexcept;
        static expressman BasicMailRequirementNotMet() noexcept;
    };
}