#pragma once

#include <ozo/pg/definitions.h>
#include <boost/hana/ext/std/tuple.hpp>
#include <tuple>

/**
 * @defgroup group-ext-std-tuple std::tuple
 * @ingroup group-ext-std
 * @brief [std::tuple](https://en.cppreference.com/w/cpp/utility/tuple) support
 *
 *@code
#include <ozo/ext/std/tuple.h>
 *@endcode
 *
 * `std::tuple<T...>` is defined as a generic composite type. and mapped as PostgreSQL
 * `Record` type and its array.
 */
namespace ozo::definitions {

template <typename ...Ts>
struct type<std::tuple<Ts...>> : pg::type_definition<decltype("record"_s)>{};

template <typename ...Ts>
struct array<std::tuple<Ts...>> : pg::array_definition<decltype("record"_s)>{};

} // namespace ozo::definitions

namespace ozo {

template <typename ...Ts>
struct is_composite<std::tuple<Ts...>> : std::true_type {};

} // namespace ozo
