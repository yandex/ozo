#pragma once

#include <ozo/type_traits.h>
#include <list>

namespace ozo {
/**
 * @defgroup group-ext-std-list std::list
 * @ingroup group-ext-std
 * @brief [std::list](https://en.cppreference.com/w/cpp/container/list) support
 *
 *@code
#include <ozo/ext/std/list.h>
 *@endcode
 *
 * `std::list<T, Allocator>` is defined as an one dimensional #Array representation type.
 */
///@{
template <typename ...Ts>
struct is_array<std::list<Ts...>> : std::true_type {};
///@}
}
