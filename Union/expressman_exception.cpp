#include "pch.h"
#include "expressman_exception.h"

namespace HYDRA15::Union::exceptions
{
    expressman::expressman(const std::string& desp, const referee::iException_code& code) noexcept
        : referee::iExceptionBase(desp, Union::framework::libID.expressman, code)
    {
    }

    expressman expressman::make_exception(const referee::iException_code& exptCode) noexcept
    {
        return expressman(
            vslz.expressman.data(),
            exptCode
        );
    }

    // 快速创建异常的函数模板
#define make(type)                                              \
    expressman expressman::type() noexcept                        \
    {                                                           \
        return expressman(vslz.type.data(),iExptCodes.type);     \
    }

    make(InterfaceUnknownExpt);
    make(InterfaceExtensionFunctionNotImplemented);
    make(InterfaceInvalidFrameSize);
    make(InterfaceDataTooLarge);
    make(InterfaceIncompleteData);
    make(InterfaceIllegalType);

    make(FactoryUnknownExpt);
    make(FactoryContaminatedData);
    make(FactoryUnknownClass);

    make(BasicMailUnknownException);
    make(BasicMailEmptyCollector);
    make(BasicMailRequirementNotMet);

#undef make
}