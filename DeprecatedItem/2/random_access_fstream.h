#pragma once
#include "pch.h"

#include "archivist_exception.h"

namespace HYDRA15::Union::archivist
{
    // fstream 的一个包装器，支持文件的随机访问、块读写
    // 写入缓冲区后会自动将内容写回磁盘
    // 一个对象绑定到一个文件，不可更改，打开新的文件需要重新创建对象
    class random_access_fstream
    {
        const std::filesystem::path filePath;
        mutable std::fstream fstream;
        bool newFile = false;

    public:
        using byte = uint8_t;

        // 构造
    public:
        explicit random_access_fstream(const std::string& fileName);
        explicit random_access_fstream(const std::filesystem::path& path);
        explicit random_access_fstream(random_access_fstream&& other) noexcept;
        explicit random_access_fstream(const random_access_fstream&) = delete;
        explicit random_access_fstream() = delete;

        // 状态检查和操作
    public:
        bool open();
        bool is_open() const;
        bool create();
        void close();
        
        size_t size() const;    // 返回文件大小
        bool new_file() const;  // 返回文件是否是新建的
        std::filesystem::path file_path();  // 返回带路径的文件名，此文件名是打开时指定的

        // 读写操作
    public:
        std::vector<byte> read(size_t pos, size_t size) const;
        bool read(size_t pos, size_t size, byte* buf) const;
        bool write(size_t pos, const std::vector<byte>& data);
        bool write(size_t pos, const byte* pdata, size_t size);
        bool reallocate(size_t size);   // 更改文件大小

    };
}