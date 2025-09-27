#pragma once
#include "pch.h"
#include "framework.h"


namespace HYDRA15::Union::framework
{
    // 集中管理的库代码
    static struct libID
    {
        static_uint unknown = 0x00;

        static_uint typical = 0xA0;
        static_uint system = 0xA1;

        // 内部子系统库代码
        static_uint Union = 0xA00;
        static_uint referee = 0xA01;
        static_uint labourer = 0xA02;
        static_uint archivist = 0xA03;
        static_uint expressman = 0xA04;
        static_uint secretary = 0xA05;
        static_uint assistant = 0xA06;



    }libID;
}