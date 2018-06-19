#pragma once

#include <istream>

namespace ozo::detail {

struct istream : std::istream {
    using std::istream::istream;
};

class istreambuf_view : public std::streambuf {
public:
    istreambuf_view(const char* data, size_t len) {
        // Standard streambuf contains only non constant char pointers for
        // buffer. So we need to do here a const_cast. However, we do not
        // allow put back characters since we do not override pbackfail().
        auto buf = const_cast<char*>(data);
        setg(buf, buf, buf + len);
    }

protected:
    std::streamsize xsgetn(char* s, std::streamsize n) override {
        n = std::min(n, egptr() - gptr());
        std::copy(gptr(), gptr() + n, s);
        gbump(n);
        return n;
    }
};

} // namespace ozo::detail
