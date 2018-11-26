#pragma once

#include <ozo/detail/ostream.h>
#include <ozo/impl/write.h>

namespace ozo {

using detail::ostream;

template <typename T>
inline auto write(ostream& out, const T& in)
        -> Require<std::is_void_v<std::void_t<decltype(impl::write(out, in))>>, ostream&> {
    return impl::write(out, in);
}

} // namespace ozo
