#pragma once
#include "framework.h"
#include "pch.h"

#include "iExceptionBase.h"
#include "libID.h"

namespace HYDRA15::Union::exceptions
{
    class commander :public referee::iExceptionBase
    {
    public:
        static struct iException_codes
        {
            static_uint commander = 0x0000;

            // Command
            static_uint CommandUnknownExpt = 0xA00;
            static_uint CommandNoSuchCommand = 0xA01;
            
        }iExptCodes;

    private:
        static struct vslz
        {
            static_string commander = "Unknown Archivist Exception";

            // Command
            static_string CommandUnknownExpt = "Unknown Command Exception";
            static_string CommandNoSuchCommand = "No such command > {}";
        }vslz;
    public:
        commander(
            const std::string& desp = vslz.commander.data(),
            const referee::iException_code& code = iExptCodes.commander
        ) noexcept;
        commander() = delete;
        virtual ~commander() noexcept = default;

        // 快速创建异常
        static commander make_exception(const referee::iException_code& exptCode = iExptCodes.commander) noexcept;

        // Command
        static commander CommandUnknownExpt() noexcept;
        static commander NoSuchCommand(const std::string& cmdline) noexcept;


    };


}