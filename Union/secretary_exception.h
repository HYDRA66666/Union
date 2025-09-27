#pragma once
#pragma once
#include "framework.h"
#include "pch.h"

#include "iExceptionBase.h"
#include "libID.h"

namespace HYDRA15::Union::exceptions
{
    class secretary : public referee::iExceptionBase
    {
    public:
        static struct iException_codes
        {
            static_uint secretary = 0x0000;

            // 打印中心
            static_uint printCenter = 0xA00;
            static_uint printBtmMsgFull = 0xA01;
            static_uint printBtmMsgNotFound = 0xA02;
        }iExptCodes;

    private:
        static struct visualize
        {
            static_string secretary = "Unknown Secretary Exception";

            // 打印中心
            static_string printCenter = "Unknown PrintCenter Exception";
            static_string printBtmMsgFull = "PrintCenter: Bottom message limit reached";
            static_string printBtmMsgNotFound = "PrintCenter: Bottom message ID not found";
        }vslz;

    public:
        secretary(
            const std::string& desp = vslz.secretary.data(),
            const referee::iException_code& code = iExptCodes.secretary
        ) noexcept;
        secretary() = delete;
        virtual ~secretary() noexcept = default;


        // 快速创建异常
        static secretary make_exception(const referee::iException_code& exptCode) noexcept;

        static secretary PrintCenterUnknown() noexcept;
        static secretary PrintCenterBtmMsgFull() noexcept;
        static secretary PrintCenterBtmMsgNotFound() noexcept;


    };
}