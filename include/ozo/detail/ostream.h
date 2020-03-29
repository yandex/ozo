#pragma once

#include <vector>
#include <ostream>

namespace ozo::detail {

class ostream {
public:
    using traits_type = std::ostream::traits_type;
    using char_type = std::ostream::char_type;

    ostream(std::vector<char_type>& buf) : buf_(buf) {}

    ostream& write(const char_type* s, std::streamsize n) {
        buf_.insert(buf_.end(), s, s + n);
        return *this;
    }

    ostream& put(char_type ch) {
        buf_.push_back(ch);
        return *this;
    }

private:
    std::vector<char_type>& buf_;
};

} // namespace ozo::detail
