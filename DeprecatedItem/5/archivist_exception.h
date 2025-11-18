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

            // file
            static_uint file = 0xA00;
            static_uint FileNotAccessable = 0xA01;
            static_uint FileFormatIncorrect = 0xA02;
            static_uint FileFull = 0xA03;
            static_uint FileIOError = 0xA04;
            static_uint FileInvalidAccess = 0xA05;

            // loader
            static_uint loader = 0xB00;
            static_uint LoaderIncorrectData = 0xB01;
        }iExptCodes;

    private:
        static struct visualize
        {
            static_string archivist = "Arvhicist unknown exception";

            // file
            static_string FileNotAccessable = "The specified file is unaccessable";
            static_string FileFormatIncorrect = "The specificed file format is incorrect";
            static_string FileFull = "File size reached the set limit";
            static_string FileIOError = "Error occures during file io operation";
            static_string FileInvalidAccess = "The accessed target does not exist";

            // loader
            static_string LoaderIncorrectData = "Loader has recived incorrect data";

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

        static archivist FileNotAccessable() noexcept;
        static archivist FileFormatIncorrect() noexcept;
        static archivist FileFull() noexcept;
        static archivist FileIOError() noexcept;
        static archivist FileInvalidAccess() noexcept;
    };
}
