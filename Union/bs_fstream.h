#pragma once
#include "pch.h"
#include "framework.h"

#include "archivist_interfaces.h"
#include "background.h"
#include "ThreadLake.h"

namespace HYDRA15::Union::archivist
{
    /***************************** 设 计 ****************************
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

    
    class bs_fstream
    {
        using wf_mutex = std::shared_mutex;

        static constexpr size_t segQueueRecordHistoryLength = 512;
        static constexpr size_t segAsyncIOThreadCount = 4;


        /********************** 段管理器 **********************/
        // 段 加载、缓存、读写
    private:
        struct segment
        {
            wf_mutex mtx;
            std::condition_variable_any cv;
            std::atomic<long long int> usageCount = 0;
            std::vector<BYTE> data;
        };

    private:
        std::filesystem::path path;
        uint64_t segSize;
        uint64_t maxSegCount;
        uint64_t segCount;
        uint64_t segMaxCacheCount;
        std::vector<segment> segments;

        // 缓存和异步 IO 相关
    private:
        std::vector<std::atomic<uint64_t>> segQueryRecordQueue = std::vector<std::atomic<uint64_t>>(segQueueRecordHistoryLength, 0xFFFFFFFF);
        std::atomic<size_t> segQueryRecordPointer = 0;
        std::atomic<uint64_t> segCachedCount = 0;
        labourer::ThreadLake segAsyncIOThreadLake{ segAsyncIOThreadCount };
        // 用于提交至线程池的任务
        void seg_load_cache(uint64_t id);
        void seg_save_cache(uint64_t id);
        void seg_unload_cache();

        // 段 读写
    private:
        std::vector<BYTE> seg_read(uint64_t id, uint64_t offset, uint64_t size);
        void seg_write(uint64_t id, uint64_t offset, const std::vector<BYTE>& data);

        // 初始化
    private:
        void segment_manager(uint64_t segSize, uint64_t maxSegCount, uint64_t segCount, uint64_t segMaxCacheCount);

    };
}