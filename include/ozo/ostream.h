#pragma once

#include <ozo/detail/ostream.h>
#include <ozo/impl/write.h>

namespace ozo {

using detail::ostream;

template <typename T>
inline ostream& write(ostream& out, const T& in) {
    return impl::write(out, in);
}

} // namespace ozo
