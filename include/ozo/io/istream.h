#pragma once

#include <ozo/detail/istream.h>
#include <ozo/impl/read.h>

namespace ozo {

using detail::istream;

template <typename T>
inline auto read(istream& in, T& out)
        -> Require<std::is_void_v<std::void_t<decltype(impl::read(in, out))>>, istream&> {
    return impl::read(in, out);
}

} // namespace ozo
