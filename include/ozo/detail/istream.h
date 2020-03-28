#pragma once

#include <istream>

namespace ozo::detail {

class istreambuf {
    const char* i_;
    const char* last_;
public:

    constexpr istreambuf(const char* data, size_t len) noexcept
    : i_(data), last_(data + len) {}

    std::streamsize read(char* buf, std::streamsize n) noexcept {
        auto last = std::min(i_ + n, last_);
        std::copy(i_, last, buf);
        n = std::distance(i_, last);
        i_ = last;
        return n;
    }
};

class istream {
public:
    using traits_type = std::istream::traits_type;
    using char_type = std::istream::char_type;

    constexpr istream(const char* data, size_t len) noexcept
    : buf_(data, len) {}

    istream& read(char_type* buf, std::streamsize len) noexcept {
        std::streamsize nbytes = buf_.read(buf, len);
        if (nbytes != len) {
            unexpected_eof_ = true;
        }
        return *this;
    }

    traits_type::int_type get() noexcept {
        char retval;
        if (!read(&retval, 1)) {
            return traits_type::eof();
        }
        return retval;
    }

    operator bool() const noexcept { return !unexpected_eof_;}
private:
    istreambuf buf_;
    bool unexpected_eof_ = false;
};

} // namespace ozo::detail
