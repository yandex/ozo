#pragma once

#include <boost/hana.hpp>

template <const char* text, class ... Args>
auto make_query(Args&& ... args) {
    return boost::hana::make_tuple(std::forward<Args&&>(args) ...);
}
