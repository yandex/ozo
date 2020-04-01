#pragma once

#include <ozo/core/concept.h>
#include <type_traits>

namespace ozo {

/**
 * @brief None type
 * None type modelling void as ordinary type. This type is callable with return type void.
 * @ingroup group-core-types
 */
struct none_t {
    using type = void;
    template <typename ...Args>
    constexpr void operator() (Args&& ...) const noexcept {}
    template <typename ...Args>
    static constexpr void apply (Args&& ...) noexcept {}
};

template <>
struct is_time_constraint<none_t> : std::true_type {};

template <typename T>
using is_none = std::is_same<T, none_t>;

template <typename T>
inline constexpr auto IsNone = is_none<T>::value;

template <typename T>
inline constexpr bool operator == (const none_t&, const T&) { return IsNone<T>;}

template <typename T, typename = Require<!IsNone<T>>>
inline constexpr bool operator == (const T&, const none_t&) { return false;}

template <typename T>
inline constexpr bool operator != (const none_t&, const T&) { return !IsNone<T>;}

template <typename T, typename = Require<!IsNone<T>>>
inline constexpr bool operator != (const T&, const none_t&) { return true;}

/**
 * @brief None object
 * @ingroup group-core-types
 *
 * Instance of ozo::none_t type.
 */
constexpr auto none = none_t{};

} // namespace ozo
