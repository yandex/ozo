#pragma once

#include <boost/hana.hpp>

namespace libapq {

template <const char* text, class ... Args>
auto make_query(Args&& ... args) {
    return boost::hana::make_tuple(std::forward<Args&&>(args) ...);
}

} // namespace libapq
