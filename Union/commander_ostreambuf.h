#pragma once
#include "pch.h"
#include "framework.h"

#include "PrintCenter.h"

namespace HYDRA15::Union::commander
{
    // AI 生成的代码
    // 自定义输出流缓冲区，自动将缓冲区内容传递给PrintCenter，用于重定向 std::cout
    class ostreambuf : public std::streambuf {
    public:
        explicit ostreambuf(std::size_t initial_size = 256, std::size_t max_size = 65536);

    protected:
        int_type overflow(int_type ch) override;

        std::streamsize xsputn(const char* s, std::streamsize n) override;

        int sync() override;

    private:
        std::vector<char> buffer_;
        std::size_t max_size_;
        std::mutex mtx_;

        void expand_buffer();

        void flush_buffer();
    };
}