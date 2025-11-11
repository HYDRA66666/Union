#pragma once
#include "pch.h"
#include "framework.h"

#include "assistant_exception.h"

namespace HYDRA15::Union::assistant
{
    // 二进制文件，对 std::fstream 的重新包装，简化调用
    // 不支持多线程并发，并发调用行为未定义
    // 创建时绑定到文件，不可更改
    class bfstream
    {
    private:
        const std::filesystem::path path;
        mutable std::fstream file;
        bool newFile = false;

    private:
        // 工具函数
        void create();
        void open();
        void close();

    public:
        // 字节组读写
        std::vector<byte> read(size_t pos, size_t size) const;
        void write(size_t pos, const std::vector<byte>& data);
        // 兼容字节指针读写
        void read(size_t pos, size_t size, byte* pdata) const;
        void write(size_t pos, size_t size, const byte* pdata);
        // 任意类型读写
        template<typename T>
            requires std::is_trivial_v<T>
        std::vector<T> read(size_t pos, size_t count) const;
        template<typename T>
            requires std::is_trivial_v<T>
        void write(size_t pos, const std::vector<T>& data);

        // 信息和管理接口
        size_t size() const;
        void resize(size_t);
        bool is_new() const;
        bool is_open() const;

        // 只允许从文件名构造，不允许拷贝，可以移动
        bfstream() = delete;
        bfstream(const bfstream&) = delete;
        bfstream(bfstream&&);
        bfstream(const std::filesystem::path&);
    };

    template<typename T>
        requires std::is_trivial_v<T>
    std::vector<T> bfstream::read(size_t pos, size_t count) const
    {
        std::vector<T> res(count, T{});
        file.sync();
        file.seekg(pos);
        file.read(reinterpret_cast<char*>(res.data()), sizeof(T) * count);
        if (!file)
            throw exceptions::assistant::FileIOError();
        return res;
    }

    template<typename T>
        requires std::is_trivial_v<T>
    void bfstream::write(size_t pos, const std::vector<T>& data)
    {
        file.seekp(pos);
        file.write(reinterpret_cast<const char*>(data.data()), sizeof(T) * data.size());
        if (!file)
            throw exceptions::assistant::FileIOError();
    }
}