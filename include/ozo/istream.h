#pragma once

#include <ozo/detail/istream.h>
#include <ozo/impl/read.h>

namespace ozo {

using detail::istream;

template <typename T>
inline istream& read(istream& in, T& out) {
    return impl::read(in, out);
}

} // namespace ozo
