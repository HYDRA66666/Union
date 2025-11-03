#pragma once
#include "pch.h"

#include "archivist_exception.h"

namespace HYDRA15::Union::archivist
{
    // fstream 的一个包装器，支持文件的随机访问
    // 不支持多个进程访问同一个文件，行为未定义
    // 支持多个线程同时访问一个此类对象，线程安全
    // 写入缓冲区后会自动将内容写回磁盘
    class random_access_fstream
    {
        mutable std::fstream fstream;
        mutable std::mutex mtx;

    public:
        using byte = uint8_t;

        // 构造
    public:
        explicit random_access_fstream(const std::string& fileName);
        explicit random_access_fstream(const std::filesystem::path::value_type* fileName);
        explicit random_access_fstream(random_access_fstream&& other) noexcept;
        explicit random_access_fstream(const random_access_fstream&) = delete;

        // 状态检查和操作
    public:
        bool open(const std::string& fileName);
        bool open(const std::filesystem::path::value_type* fileName);
        bool is_open() const;
        bool create(const std::string& fileName);
        bool create(const std::filesystem::path::value_type* fileName);
        void close();

        size_t size() const;    // 返回文件大小

        // 读写操作
    public:
        std::vector<byte> read(size_t pos, size_t size) const;
        bool read(char* buf, size_t pos, size_t size) const;
        bool write(size_t pos, const std::vector<byte>& data);
    };
}