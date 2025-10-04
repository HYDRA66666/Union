#include "pch.h"
#include "commander_ostreambuf.h"

namespace HYDRA15::Union::commander
{
    ostreambuf::ostreambuf(std::size_t initial_size, std::size_t max_size)
        : buffer_(initial_size), max_size_(max_size) 
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
            secretary::PrintCenter::println(std::string(pbase(), n));
            pbump(static_cast<int>(-n));
        }
    }






}