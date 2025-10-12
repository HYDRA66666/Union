#include "pch.h"
#include "secretary_streambuf.h"

namespace HYDRA15::Union::secretary
{
    ostreambuf::ostreambuf(std::function<void(const std::string&)> c, std::size_t initial_size, std::size_t max_size)
        : buffer_(initial_size), max_size_(max_size), callback(c)
    {
        setp(buffer_.data(), buffer_.data() + buffer_.size());
    }

    ostreambuf::int_type ostreambuf::overflow(int_type ch)
    {
        std::lock_guard lg(mtx_);
        if (ch == traits_type::eof()) return traits_type::not_eof(ch);

        if (pptr() == epptr()) {
            if (buffer_.size() < max_size_) {
                expand_buffer();
            }
            else {
                flush_buffer();
            }
        }
        *pptr() = ch;
        pbump(1);

        if (ch == '\n') {
            flush_buffer();
        }
        return ch;
    }

    std::streamsize ostreambuf::xsputn(const char* s, std::streamsize n)
    {
        std::lock_guard lg(mtx_);
        std::streamsize written = 0;
        while (written < n) {
            std::size_t space_left = epptr() - pptr();
            if (space_left == 0) {
                if (buffer_.size() < max_size_) {
                    expand_buffer();
                    space_left = epptr() - pptr();
                }
                else {
                    flush_buffer();
                    space_left = epptr() - pptr();
                }
            }
            std::size_t to_write = std::min<std::size_t>(space_left, n - written);

            const char* nl = static_cast<const char*>(memchr(s + written, '\n', to_write));
            if (nl) {
                std::size_t nl_pos = nl - (s + written);
                std::memcpy(pptr(), s + written, nl_pos + 1);
                pbump(static_cast<int>(nl_pos + 1));
                written += nl_pos + 1;
                flush_buffer();
            }
            else {
                std::memcpy(pptr(), s + written, to_write);
                pbump(static_cast<int>(to_write));
                written += to_write;
            }
        }
        return written;
    }

    int ostreambuf::sync()
    {
        std::lock_guard lg(mtx_);
        flush_buffer();
        return 0;
    }

    void ostreambuf::expand_buffer()
    {
        std::lock_guard lg(mtx_);
        std::size_t current_size = buffer_.size();
        std::size_t new_size = std::min(current_size * 2, max_size_);
        std::ptrdiff_t offset = pptr() - pbase();
        buffer_.resize(new_size);
        setp(buffer_.data(), buffer_.data() + buffer_.size());
        pbump(static_cast<int>(offset));
    }

    void ostreambuf::flush_buffer()
    {
        std::ptrdiff_t n = pptr() - pbase();
        if (n > 0) {
            callback(std::string(pbase(), n));
            pbump(static_cast<int>(-n));
        }
    }

    bool istreambuf::refill()
    {
        if (eof_) return false;

        buffer_ = std::move(getline_callback());
        if (buffer_.empty()) {
            // 假设返回空字符串表示输入结束（EOF）
            eof_ = true;
            return false;
        }

        // 将整行 + 换行符存入 buffer_
        // 注意：即使原输入无 '\n'，我们也添加一个，以模拟标准输入行为
        buffer_ += '\n';

        // 设置 get area: [begin, end)
        char* beg = &buffer_[0];
        char* end = beg + buffer_.size();
        setg(beg, beg, end);

        return true;
    }

    istreambuf::istreambuf(std::function<std::string()> callback)
        : getline_callback(std::move(callback))
    {
        setg(nullptr, nullptr, nullptr); // 初始化 get area
    }

    int istreambuf::underflow()
    {
        if (gptr() < egptr()) {
            return traits_type::to_int_type(*gptr());
        }
        if (!refill()) {
            return traits_type::eof();
        }
        return traits_type::to_int_type(*gptr());
    }

    std::streamsize istreambuf::xsgetn(char* s, std::streamsize count)
    {
        std::streamsize total = 0;

        while (total < count) {
            // 当前缓冲区中可用字符数
            std::streamsize avail = egptr() - gptr();
            if (avail > 0) {
                std::streamsize to_copy = std::min(avail, count - total);
                std::copy(gptr(), gptr() + to_copy, s + total);
                gbump(static_cast<int>(to_copy)); // 移动 get 指针
                total += to_copy;
            }
            else {
                // 缓冲区为空，尝试 refill
                if (!refill()) {
                    // EOF：返回已读取的字节数（可能为 0）
                    break;
                }
                // refill 成功后，下一轮循环会复制数据
            }
        }

        return total;
    }

    std::streamsize istreambuf::showmanyc()
    {
        if (gptr() < egptr()) {
            return egptr() - gptr();
        }
        return eof_ ? -1 : 0; // -1 表示 EOF
    }

}