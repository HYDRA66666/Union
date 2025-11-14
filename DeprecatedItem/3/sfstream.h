#pragma once
#include "pch.h"
#include "framework.h"

#include "archivist_interfaces.h"
#include "background.h"
#include "ThreadLake.h"
#include "files.h"
#include "basic_blockable_queue.h"

namespace HYDRA15::Union::archivist
{
    /***************************** 设 计 ****************************
    * 以 段 为文件的最小 IO 单位，LRU 段缓存机制，多线程 IO
    * 用户以 节 为文件分割单位，一个节由数个连续或不连续的段组成
    * 用户访问文件时，只需传入节名、节内偏移量和大小，程序自动管理段节映射
    * 自动多路 IO ，自动节缓存
    * 
    * **************************** 文件格式 ****************************
    * 根节（第 0 段开始）：
    *   16 "ArchvstSegmented" 标记    8 版本号    8 段大小
    *   8 最大段数    8 已用段数    8 自定义头开始指针    8 自定义头长度
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

    
    class sfstream
    {
        /***************************** 段管理器 ****************************/
        // 段读写、扩展，多路 IO
    private:
        class segment_mamager : protected labourer::background
        {
        private: // 参数、类型和结构
            static constexpr unsigned int segIOThreadCount = 4; // io 线程数
            using read_result = std::expected<std::vector<byte>, std::exception_ptr>;
            struct seg_io_mission
            {
                enum class operation { nothing = 0, read, write } oper = operation::nothing;
                uint64_t segID = 0;
                std::promise<read_result> readPrms{};
                std::vector<byte> writePrms{};
            };

        private: // 数据
            const std::filesystem::path path;
            uint64_t segSize;
            uint64_t maxSegs;
            // 多线程 IO
            labourer::basic_blockable_queue<seg_io_mission> queue;
            std::atomic_bool working = true;

        private: // 内部函数
            virtual void work(thread_info& info) override;

        public: // 接口
            // 创建
            segment_mamager() = delete;
            segment_mamager(const std::filesystem::path& path);
            ~segment_mamager();
            using background::start;

            // 读写
            std::vector<byte> read(const std::list<uint64_t>& segIDs);                      // 读一系列段，将数据连续地返回
            void write(const std::list<uint64_t>& segIDs, const std::vector<byte>& data);   // 写一系列段，传入段编号列表和连续的数据
            uint64_t expand(uint64_t newSegCnt);                // 扩展 newSegCnt 个段，写入全 0 数据，返回扩展的第一个段的 ID

            // 信息与控制
            uint64_t seg_size() const;  // 获取段大小
            void seg_size(uint64_t);    // 设置段大小
            uint64_t max_segs() const;  // 获取最大段数
            void max_segs(uint64_t);    // 设置最大段数
            uint64_t used_segs() const; // 获取已用段数
            void used_segs(uint64_t);   // 设置已用段数
        };



        /***************************** 节管理器 ****************************/
    private:
        struct section
        {
            std::deque<uint64_t> segIDLst;
            uint64_t cachedSegNoStart, cachedSegNoEnd;
            std::vector<byte> cache;
        };
    };
}