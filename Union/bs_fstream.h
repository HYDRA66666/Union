#pragma once
#include "pch.h"
#include "framework.h"

#include "archivist_interfaces.h"
#include "background.h"

namespace HYDRA15::Union::archivist
{
    /***************************** 设 计 ****************************
    * binary segmented fstream：
    * 以 段 为文件的最小 IO 单位，LRU 段缓存机制，多线程 IO
    * 用户以 节 为文件分割单位，一个节由数个连续或不连续的段组成
    * 用户访问文件时，只需传入节名、节内偏移量和大小，程序自动管理段节映射
    * 
    * 
    * **************************** 文件格式 ****************************
    * 根节（第 0 段开始）：
    *   16 "ArchvstSegmented" 标记    8 版本号    8 保留
    *   8 段大小    8 最大段数    8 已用段数    8 保留
    *   8 根节段表指针    8 根节段数    8 节表起始指针    8 节数
    *   32N 可选额外信息
    *   32N 根节段表：
    *       8N 段 ID
    *   32N 可选额外信息
    *   32N 节表：
    *       8 节名指针    8 节注释指针    8 段表指针    8 段数
    *   32N 节名数据
    *   32N 节注释数据
    *   32N 段表数据
    * 
    */

}