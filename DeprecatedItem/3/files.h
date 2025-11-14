#pragma once
#include "pch.h"
#include "framework.h"

#include "assistant_exception.h"

namespace HYDRA15::Union::assistant
{
    // 二进制文件流，提供从二进制数据中快速读取各种平凡类型的功能
    class bfstream
    {
        const std::filesystem::path path;
        mutable std::fstream file;
        bool newFile = false;
        mutable std::mutex mtx;

    private:
        // 包装工具函数
        void create();
        void open();
        void close();

    public:
        using byte = uint8_t;   // 字节类型

        bfstream() = delete;
        bfstream(const std::filesystem::path&); // 只能从文件路径构建
        bfstream(bfstream&&);

        // 控制和信息接口
        bool is_new_file() const;
        bool is_open() const;
        size_t size() const;
        void resize(size_t);

        // 读写接口
        void read(size_t pos, size_t size, byte* buf);
        void write(size_t pos, size_t size, const byte* buf);
        template<typename T>
            requires std::is_trivial_v<T>
        T read(size_t pos) const;
        template<typename T>
            requires std::is_trivial_v<T>
        void write(size_t pos, const T& t);
        template<typename T>
            requires std::is_trivial_v<T>
        std::vector<T> read_array(size_t pos, size_t count);
        template<typename T>
            requires std::is_trivial_v<T>
        void write_array(size_t pos, const std::vector<T>& data);
    };

    template<typename T>
        requires std::is_trivial_v<T>
    T bfstream::read(size_t pos) const
    {
        std::unique_lock ul{ mtx };
        T t{};
        file.sync();
        file.seekg(pos);
        file.read(reinterpret_cast<char*>(&t), sizeof(T));
        if (!file)
            throw exceptions::assistant::FileIOError();
        return t;
    }

    template<typename T>
        requires std::is_trivial_v<T>
    void bfstream::write(size_t pos, const T& t)
    {
        std::unique_lock ul{ mtx };
        file.seekp(pos);
        file.write(reinterpret_cast<const char*>(&t), sizeof(T));
        file.flush();
        if (!file)
            throw exceptions::assistant::FileIOError();
    }

    template<typename T>
        requires std::is_trivial_v<T>
    std::vector<T> bfstream::read_array(size_t pos, size_t count)
    {
        std::unique_lock ul{ mtx };
        std::vector<T> res;
        res.resize(count);
        file.sync();
        file.seekg(pos);
        file.read(reinterpret_cast<char*>(res.data()), sizeof(T) * count);
        if (!file)
            throw exceptions::assistant::FileIOError();
        return res;
    }

    template<typename T>
        requires std::is_trivial_v<T>
    void bfstream::write_array(size_t pos, const std::vector<T>& data)
    {
        std::unique_lock ul{ mtx };
        file.seekp(pos);
        file.write(reinterpret_cast<const char*>(data.data()), sizeof(T) * data.size());
        if (!file)
            throw exceptions::assistant::FileIOError();
    }

}