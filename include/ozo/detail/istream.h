#pragma once

#include <ozo/concept.h>
#include <ozo/detail/endian.h>
#include <ozo/detail/float.h>
#include <istream>

namespace ozo::detail {

using std::istream;

class istreambuf_view : public std::streambuf {
public:
    istreambuf_view(const char* data, size_t len) :
        begin_(data), end_(data + len), current_(data)
    {}
protected:
    int_type underflow() override {
        return (current_ == end_ ? traits_type::eof() : traits_type::to_int_type(*current_));
    }

    int_type uflow() override {
        return (current_ == end_ ? traits_type::eof() : traits_type::to_int_type(*current_++));
    }

    int_type pbackfail(int_type ch) override {
        if (current_ == begin_ || (ch != traits_type::eof() && ch != current_[-1])) {
            return traits_type::eof();
        }
        return traits_type::to_int_type(*--current_);
    }

    std::streamsize showmanyc() override {
        return end_ - current_;
    }

    const char* const begin_;
    const char* const end_;
    const char* current_;
};

} // namespace ozo::detail
