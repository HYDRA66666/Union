#pragma once
#include "pch.h"
#include "framework.h"


namespace HYDRA15::Union::secretary
{
    // AI 生成的代码
    // 自定义输出流缓冲区，自动将缓冲区内容传递给PrintCenter，用于重定向 std::cout
    class ostreambuf : public std::streambuf {
    public:
        explicit ostreambuf(std::function<void(const std::string&)> callback, std::size_t initial_size = 256, std::size_t max_size = 65536);

    protected:
        int_type overflow(int_type ch) override;

        std::streamsize xsputn(const char* s, std::streamsize n) override;

        int sync() override;

    private:
        std::vector<char> buffer_;
        std::size_t max_size_;
        std::mutex mtx_;
        std::function<void(const std::string&)> callback;

        void expand_buffer();

        void flush_buffer();
    };

    // AI生成的代码
    // 自定义输入流缓冲区，用于将输入重定向至 Command 类，用于重定向 std::cin
    class istreambuf : public std::streambuf 
    {
    private:
        std::string buffer_;   // 使用 std::string 作为缓冲区（自动管理内存）
        bool eof_ = false;     // 是否已到逻辑 EOF

        // 回调函数：用于获取一行输入
        std::function<std::string()> getline_callback;

        // 从 Command::getline() 获取一行并加载到 buffer_
        bool refill();

    public:
        // 构造时传入回调
        explicit istreambuf(std::function<std::string()> callback);

        // 重写 underflow：单字符读取时调用
        int underflow() override;

        // 1. 优化批量读取：重写 xsgetn
        std::streamsize xsgetn(char* s, std::streamsize count) override;

        // 2. 支持 cin.getline()：确保换行符被正确消费
        //    标准要求：getline 会读取直到 '\n' 或缓冲区满，并丢弃 '\n'
        //    我们的 underflow/xsgetn 已提供 '\n'，因此无需额外处理
        //    但需确保：当遇到 EOF 且无 '\n' 时，仍能正确终止
        //    —— 这由标准库的 istream::getline 逻辑处理，我们只需提供字符流

        // 可选：重写 showmanyc() 以提示可用字符数（非必需，但可优化）
        std::streamsize showmanyc() override;

    };
}