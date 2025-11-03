#include "pch.h"
#include "archivist_exception.h"

namespace HYDRA15::Union::exceptions
{
    archivist::archivist(const std::string& desp, const referee::iException_code& code) noexcept
        :referee::iExceptionBase(desp, framework::libID.assistant, code)
    {
    }

    archivist archivist::make_exception(const referee::iException_code& code) noexcept
    {
        return archivist(vslz.archivist.data(), code);
    }

    // 快速创建异常的函数模板
#define make(type)                                              \
    archivist archivist::type() noexcept                        \
    {                                                           \
        return archivist(vslz.type.data(),iExptCodes.type);     \
    }

    make(RAFstreamFileNotAccessable);
    make(FileFormatIncorrect);

#undef make



}
