#pragma once
#include "pch.h"

#include "archivist_interfaces.h"

namespace HYDRA15::Union::archivist
{
    /* 设计：
    *   内存表将会始终尝试将所有数据加载至内存中，除非达到内存容量的限制
    *   如果达到给定的内存容量限制，表将会采用最近访问原则将热点数据保留在内存上
    *   数据加载按页进行，页大小由加载器决定
    *   任何操作都不会直接作用于磁盘
    */
}