#pragma once
#include "framework.h"
#include "pch.h"

#include "iExceptionBase.h"
#include "libID.h"

namespace HYDRA15::Union::exceptions
{
    class labourer : public referee::iExceptionBase
    {
    public:
        static struct iException_codes
        {
            static_uint labourer = 0x000;
        }iExptCodes;

    private:
        static struct visualize
        {
            static_string labourer = "Labourer unknown exception";
        }vslz;

    public:
        labourer(const std::string& desp, const referee::iException_code& code) noexcept;
        labourer() = delete;
        virtual ~labourer() noexcept = default;


        // 快速创建异常
        static labourer make_exception(const referee::iException_code& exptCode) noexcept;

    };
}