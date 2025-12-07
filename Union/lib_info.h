#pragma once
#include "pch.h"
#include "framework.h"

#include "astring.h"


namespace HYDRA15::Union::framework
{
    static_string libName = "HYDRA15.Union";
    static_string version = "ver.lib.release.1.2.1";

    // 子系统代码
    static struct libID
    {
        static_uint Union = 0x000000;
        static_uint referee = 0x0A0000;
        static_uint assistant = 0x0B0000;
        static_uint labourer = 0x0C0000;
        static_uint secretary = 0x0D0000;
    }libID;

}