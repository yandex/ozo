#pragma once

#include <ozo/type_traits.h>
#include <vector>

namespace ozo {
/**
 * @defgroup group-ext-std-vector std::vector
 * @ingroup group-ext-std
 * @brief [std::vector](https://en.cppreference.com/w/cpp/container/vector) support
 *
 *@code
#include <ozo/ext/std/vector.h>
 *@endcode
 *
 * `std::vector<T, Allocator>` is declared as the one dimensional array representation type.
 * @models{Array}
 */
///@{
template <typename ...Ts>
struct is_array<std::vector<Ts...>> : std::true_type {};
///@}
}
