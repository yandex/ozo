#pragma once

#include <ozo/pg/definitions.h>
#include <utility>
#include <boost/hana/ext/std/pair.hpp>

/**
 * @defgroup group-ext-std-pair std::pair
 * @ingroup group-ext-std
 * @brief [std::pair](https://en.cppreference.com/w/cpp/utility/pair) support
 *
 *@code
#include <ozo/ext/std/pair.h>
 *@endcode
 *
 * `std::pair<T1, T2>` is defined as a generic composite type of two elements
 * and mapped as PostgreSQL `Record` type and its array.
 */
namespace ozo::definitions {

template <typename ...Ts>
struct type<std::pair<Ts...>> : pg::type_definition<decltype("record"_s)>{};

template <typename ...Ts>
struct array<std::pair<Ts...>> : pg::array_definition<decltype("record"_s)>{};

} // namespace ozo::definitions

namespace ozo {

template <typename ...Ts>
struct is_composite<std::pair<Ts...>> : std::true_type {};

} // namespace ozo
