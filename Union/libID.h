#pragma once
#include "pch.h"
#include "framework.h"

#include "astring.h"


namespace HYDRA15::Union::framework
{
    static_string libName = "HYDRA15.Union";
    static_string version = "lib-alpha-0.1.1";

    // 子系统代码
    static struct libID
    {
        static_uint Union = 0xA00;
        static_uint referee = 0xA01;
        static_uint labourer = 0xA02;
        static_uint archivist = 0xA03;
        static_uint expressman = 0xA04;
        static_uint secretary = 0xA05;
        static_uint assistant = 0xA06;
        static_uint commander = 0xA07;

    }libID;
}