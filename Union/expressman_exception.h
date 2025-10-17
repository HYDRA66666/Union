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

            // 接口
            static_uint InterfaceUnknownExpt = 0xA00;
            static_uint InterfaceExtensionFunctionNotImplemented = 0xA01;
            static_uint InterfaceInvalidFrameSize = 0xA02;
            static_uint InterfaceDataTooLarge = 0xA03;
            static_uint InterfaceIncompleteData = 0xA04;
            static_uint InterfaceIllegalType = 0xA05;

            // 工厂
            static_uint FactoryUnknownExpt = 0xB00;
            static_uint FactoryContaminatedData = 0xB01;
            static_uint FactoryUnknownClass = 0xB02;

            // mail
            static_uint BasicMailUnknownException = 0xC00;
            static_uint BasicMailEmptyCollector = 0xC01;
            static_uint BasicMailRequirementNotMet = 0xC02;
        }iExptCodes;

    private:
        static struct vslz
        {
            static_string expressman = "Unknown Expressman Exception";

            // 接口
            static_string InterfaceUnknownExpt = "Unknown Interface exception";
            static_string InterfaceExtensionFunctionNotImplemented = "This extension function is not implemented";
            static_string InterfaceInvalidFrameSize = "Prama maxFrameSize is invalid";
            static_string InterfaceDataTooLarge = "The data to be packaged is too large";
            static_string InterfaceIncompleteData = "The obtained data is incomplete";
            static_string InterfaceIllegalType = "Input type not allowed";

            // 工厂
            static_string FactoryUnknownExpt = "Unknown Factory Exception";
            static_string FactoryContaminatedData = "The archive list contains archives from more than one class";
            static_string FactoryUnknownClass = "The constructorof specific class is not registered";

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

        // 接口
        static expressman InterfaceUnknownExpt() noexcept;
        static expressman InterfaceExtensionFunctionNotImplemented() noexcept;
        static expressman InterfaceInvalidFrameSize() noexcept;
        static expressman InterfaceDataTooLarge() noexcept;
        static expressman InterfaceIncompleteData() noexcept;
        static expressman InterfaceIllegalType() noexcept;

        // 工厂
        static expressman FactoryUnknownExpt() noexcept;
        static expressman FactoryContaminatedData() noexcept;
        static expressman FactoryUnknownClass() noexcept;

        // basic mail
        static expressman BasicMailUnknownException() noexcept;
        static expressman BasicMailEmptyCollector() noexcept;
        static expressman BasicMailRequirementNotMet() noexcept;
    };
}