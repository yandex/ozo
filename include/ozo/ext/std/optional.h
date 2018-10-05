#pragma once

#include <ozo/type_traits.h>
#include <ozo/optional.h>

namespace ozo {

#ifdef __OZO_STD_OPTIONAL
template <typename T>
struct is_nullable<__OZO_STD_OPTIONAL<T>> : std::true_type {};
#endif

} // namespace ozo
