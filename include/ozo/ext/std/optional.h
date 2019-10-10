#pragma once

#include <ozo/type_traits.h>
#include <ozo/optional.h>

namespace ozo {
/**
 * @defgroup group-ext-std-optional std::optional
 * @ingroup group-ext-std
 * @brief [std::optional](https://en.cppreference.com/w/cpp/utility/optional) type support
 *
 *@code
#include <ozo/ext/std/optional.h>
 *@endcode
 *
 * The `std::optional<T>` type is defined as #Nullable and uses default
 * implementation of related functions.
 *
 * The `ozo::unwrap()` function is implemented via the dereference operator.
 */
///@{
#ifdef OZO_STD_OPTIONAL
template <typename T>
struct is_nullable<OZO_STD_OPTIONAL<T>> : std::true_type {};

template <typename T>
struct unwrap_impl<OZO_STD_OPTIONAL<T>> : detail::functional::dereference {};
#endif
///@}
} // namespace ozo
