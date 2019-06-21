#pragma once

#include <ozo/pg/definitions.h>
#include <ozo/optional.h>
#include <ozo/io/send.h>
#include <ozo/detail/functional.h>

namespace ozo {
/**
 * @defgroup group-ext-std-nullopt std::nullopt_t
 * @ingroup group-ext-std
 * @brief [std::nullopt](https://en.cppreference.com/w/cpp/utility/optional/nullopt) support
 *
 *@code
#include <ozo/ext/std/nullopt_t.h>
 *@endcode
 *
 * The `std::nullopt_t` type is defined as #Nullable which is always in null state.
 *
 * The `ozo::unwrap()` function with `std::nullopt_t` type argument returns `std::nullopt`.
 *
 * The `std::nullopt_t` type is mapped as `NULL` for PostgreSQL.
 */
///@{
template <>
struct is_nullable<__OZO_NULLOPT_T> : std::true_type {};

template <>
struct unwrap_impl<__OZO_NULLOPT_T> : detail::functional::forward {};

template <>
struct is_null_impl<__OZO_NULLOPT_T> : detail::functional::always_true {};

template <>
struct send_impl<__OZO_NULLOPT_T> {
    template <typename M, typename T>
    static ostream& apply(ostream& out, M&&, T&&) { return out;}
};
///@}
} // namespace ozo

OZO_PG_BIND_TYPE(__OZO_NULLOPT_T, "null")
