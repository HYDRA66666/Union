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
        void create()
        {
            std::filesystem::path parentPath = path.parent_path();

            // 创建指定路径
            if (!parentPath.empty() && !std::filesystem::exists(parentPath))
                std::filesystem::create_directories(parentPath);

            // 检查并创建文件
            if (std::filesystem::exists(path))
                return;
            std::ofstream ofs(path);
            if (!ofs)
                throw exceptions::assistant::FileNotAccessable();

            newFile = true;
        }

        void open()
        {
            file.open(path, std::ios::in | std::ios::out | std::ios::binary);
            if (!file.is_open())
                throw exceptions::assistant::FileNotAccessable();
        }

        void close(){ file.close(); }


    public:
        // 字节组读写
        std::vector<byte> read(size_t pos, size_t size) const
        {
            std::vector<byte> data(size, 0);
            file.sync();
            file.seekg(pos);
            file.read(reinterpret_cast<char*>(data.data()), size);
            if (!file)
                throw exceptions::assistant::FileIOError();
            return data;
        }

        void write(size_t pos, const std::vector<byte>& data)
        {
            file.seekp(pos);
            file.write(reinterpret_cast<const char*>(data.data()), data.size());
            file.flush();
            if (!file)
                throw exceptions::assistant::FileIOError();
        }

        // 兼容字节指针读写
        void read(size_t pos, size_t size, byte* pdata) const
        {
            file.sync();
            file.seekg(pos);
            file.read(reinterpret_cast<char*>(pdata), size);
            if (!file)
                throw exceptions::assistant::FileIOError();
        }

        void write(size_t pos, size_t size, const byte* pdata)
        {
            file.seekp(pos);
            file.write(reinterpret_cast<const char*>(pdata), size);
            file.flush();
            if (!file)
                throw exceptions::assistant::FileIOError();
        }

        // 任意类型读写
        template<typename T>
            requires std::is_trivial_v<T>
        std::vector<T> read(size_t pos, size_t count) const
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
        void write(size_t pos, const std::vector<T>& data)
        {
            file.seekp(pos);
            file.write(reinterpret_cast<const char*>(data.data()), sizeof(T) * data.size());
            if (!file)
                throw exceptions::assistant::FileIOError();
        }

        // 信息和管理接口
        size_t size() const { file.seekg(0, std::ios::end); return file.tellg(); }

        void resize(size_t size)
        {
            file.close();
            std::error_code ec;
            std::filesystem::resize_file(path, size, ec);
            if (ec)
                throw exceptions::assistant::FileIOError(ec);
            file.open(path);
        }
        bool is_new() const { return newFile; }
        bool is_open() const { return file.is_open(); }

        // 只允许从文件名构造，不允许拷贝，可以移动
        bfstream() = delete;
        bfstream(const bfstream&) = delete;
        bfstream(bfstream&& oth)noexcept :path(oth.path), file(std::move(oth.file)){}
        bfstream(const std::filesystem::path& p)
            :path(p)
        {
            if (!std::filesystem::exists(path))
                create();
            open();
        }
    };
}