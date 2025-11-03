#pragma once
#include "pch.h"

#include "iExceptionBase.h"
#include "libID.h"

namespace HYDRA15::Union::exceptions
{
    class archivist : public referee::iExceptionBase
    {
    public:
        static struct iExceptionCodes
        {
            static_uint archivist = 0x000;

            // random_access_fstream
            static_uint random_access_fstream = 0xA00;
            static_uint RAFstreamFileNotAccessable = 0xA01;

            // loader
            static_uint loader = 0xB00;
            static_uint FileFormatIncorrect = 0xB01;
        }iExptCodes;

    private:
        static struct visualize
        {
            static_string archivist = "Arvhicist unknown exception";

            // random_access_fstream
            static_string RAFstreamFileNotAccessable = "The specified file is unreachable";

            // loader
            static_string FileFormatIncorrect = "The specificed file format is incorrect";
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

        static archivist RAFstreamFileNotAccessable() noexcept;
        static archivist FileFormatIncorrect() noexcept;
    };
}
