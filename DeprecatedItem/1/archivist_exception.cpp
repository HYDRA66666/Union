#include "pch.h"
#include "archivist_exception.h"

namespace HYDRA15::Union::exceptions
{
    archivist::archivist(const std::string& desp, const referee::iException_code& code) noexcept
        : referee::iExceptionBase(desp, Union::framework::libID.archivist, code)
    {
    }

    archivist archivist::make_exception(const referee::iException_code& exptCode) noexcept
    {
        return archivist(
            vslz.archivist.data(),
            exptCode
        );
    }

    // 快速创建异常的函数模板
#define make(type)                                              \
    archivist archivist::type() noexcept                        \
    {                                                           \
        return archivist(vslz.type.data(),iExptCodes.type);     \
    }

    make(RegistryUnknownExpt);
    make(RegistryTabletInvalidMaxSize);
    make(RegistryTabletFull);
    make(RegistryKeyExists);
    make(RegistryKeyNotFound);

    make(InterfaceUnknownExpt);
    make(InterfaceExtensionFunctionNotImplemented);
    make(InterfaceInvalidFrameSize);
    make(InterfaceDataTooLarge);
    make(InterfaceIncompleteData);
    make(InterfaceIllegalType);

    make(FactoryUnknownExpt);
    make(FactoryContaminatedData);
    make(FactoryUnknownClass);

#undef make
}