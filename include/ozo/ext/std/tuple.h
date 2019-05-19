#pragma once

#include <ozo/type_traits.h>
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
struct type<std::tuple<Ts...>> :
    detail::type_definition<decltype("record"_s), oid_constant<RECORDOID>>{};

template <typename ...Ts>
struct array<std::tuple<Ts...>> :
    detail::array_definition<std::tuple<Ts...>, oid_constant<RECORDARRAYOID>>{};

} // namespace ozo::definitions

namespace ozo {

template <typename ...Ts>
struct is_composite<std::tuple<Ts...>> : std::true_type {};

} // namespace ozo

