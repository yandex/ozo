#pragma once

#include <ozo/type_traits.h>
#include <boost/optional.hpp>

namespace ozo {
/**
 * @defgroup group-ext-boost-optional boost::optional
 * @ingroup group-ext-boost
 * @brief [Boost.Optional](https://www.boost.org/doc/libs/1_69_0/libs/optional/doc/html/index.html) library support
 *
 *@code
#include <ozo/ext/boost/optional.h>
 *@endcode
 *
 * The `boost::optional<T>` type is defined as #Nullable and uses default
 * implementation of related functions.
 *
 * The `ozo::unwrap()` function is implemented via the dereference operator.
 */
///@{
template <typename T>
struct is_nullable<boost::optional<T>> : std::true_type {};

template <typename T>
struct unwrap_impl<boost::optional<T>> : detail::functional::dereference {};
///@}
} // namespace ozo

