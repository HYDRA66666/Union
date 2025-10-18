#pragma once
#include "framework.h"
#include "pch.h"

#include "iExceptionBase.h"
#include "libID.h"

namespace HYDRA15::Union::exceptions
{
    class archivist :public referee::iExceptionBase
    {
    public:
        static struct iException_codes
        {
            static_uint archivist = 0x0000;

            // 注册机
            static_uint RegistryUnknownExpt = 0xA00;
            static_uint RegistryTabletInvalidMaxSize = 0xA01;
            static_uint RegistryTabletFull = 0xA02;
            static_uint RegistryKeyExists = 0xA03;
            static_uint RegistryKeyNotFound = 0xA04;

            // 接口
            static_uint InterfaceUnknownExpt = 0xB00;
            static_uint InterfaceExtensionFunctionNotImplemented = 0xB01;
            static_uint InterfaceInvalidFrameSize = 0xB02;
            static_uint InterfaceDataTooLarge = 0xB03;
            static_uint InterfaceIncompleteData = 0xB04;
            static_uint InterfaceIllegalType = 0xB05;

            // 工厂
            static_uint FactoryUnknownExpt = 0xC00;
            static_uint FactoryContaminatedData = 0xC01;
            static_uint FactoryUnknownClass = 0xC02;
        }iExptCodes;

    private:
        static struct vslz
        {
            static_string archivist = "Unknown Archivist Exception";

            // 注册机
            static_string RegistryUnknownExpt = "Unknown Registry Exception";
            static_string RegistryTabletInvalidMaxSize = "Registry: Invalid max size for Registry";
            static_string RegistryTabletFull = "Registry: Registry is full";
            static_string RegistryKeyExists = "Registry: Key already exists in Registry";
            static_string RegistryKeyNotFound = "Registry: Key not found in Registry";

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
        }vslz;
    public:
        archivist(
            const std::string& desp = vslz.archivist.data(),
            const referee::iException_code& code = iExptCodes.archivist
        ) noexcept;
        archivist() = delete;
        virtual ~archivist() noexcept = default;

        // 快速创建异常
        static archivist make_exception(const referee::iException_code& exptCode = iExptCodes.archivist) noexcept;

        // 注册机
        static archivist RegistryUnknownExpt() noexcept;
        static archivist RegistryTabletInvalidMaxSize() noexcept;
        static archivist RegistryTabletFull() noexcept;
        static archivist RegistryKeyExists() noexcept;
        static archivist RegistryKeyNotFound() noexcept;

        // 接口
        static archivist InterfaceUnknownExpt() noexcept;
        static archivist InterfaceExtensionFunctionNotImplemented() noexcept;
        static archivist InterfaceInvalidFrameSize() noexcept;
        static archivist InterfaceDataTooLarge() noexcept;
        static archivist InterfaceIncompleteData() noexcept;
        static archivist InterfaceIllegalType() noexcept;

        // 工厂
        static archivist FactoryUnknownExpt() noexcept;
        static archivist FactoryContaminatedData() noexcept;
        static archivist FactoryUnknownClass() noexcept;
    };


}