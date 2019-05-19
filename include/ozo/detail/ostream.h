#pragma once

#include <vector>
#include <ostream>

namespace ozo::detail {

struct ostream : std::ostream {
    using std::ostream::ostream;
};

class ostreambuf : public std::streambuf {
    std::vector<char_type>& buf_;

public:
    ostreambuf(std::vector<char_type>& buf) : buf_(buf) {}

protected:
    std::streamsize xsputn(const char_type* s, std::streamsize n) override {
        buf_.insert(buf_.end(), s, s + n);
        return n;
    }

    int_type overflow(int_type ch) override {
        using traits = std::char_traits<char_type>;
        if (!traits::eq_int_type(ch, traits::eof())) {
            buf_.push_back(static_cast<char_type>(ch));
        }
        return ch;
    }
};

} // namespace ozo::detail

