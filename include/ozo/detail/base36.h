#pragma once

#include <boost/range/algorithm/reverse.hpp>
#include <string_view>
#include <string>

namespace ozo::detail {

inline long b36tol(std::string_view in) {
    return strtol(in.data(), nullptr, 36);
}

inline std::string ltob36(long i) {
    auto u = static_cast<unsigned long>(i);
    std::string out ;
    constexpr auto base = 36;
    do {
        const char d = u % base;
        out.push_back(d < 10 ? '0' + d : 'A' + d - 10);
        u /= base;
    } while (u > 0);

    boost::range::reverse(out);
    return out;
}

} // namespace ozo::detail
