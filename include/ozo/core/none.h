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

/**
 * @brief None object
 * @ingroup group-core-types
 *
 * Instance of ozo::none_t type.
 */
constexpr auto none = none_t{};

template <>
struct is_time_constraint<none_t> : std::true_type {};

inline constexpr std::true_type operator == (const none_t&, const none_t&) { return {};}

template <typename T>
inline constexpr std::false_type operator == (const none_t&, const T&) { return {};}

template <typename T>
inline constexpr std::false_type operator == (const T&, const none_t&) { return {};}

inline constexpr std::false_type operator != (const none_t&, const none_t&) { return {};}

template <typename T>
inline constexpr std::true_type operator != (const none_t&, const T&) { return {};}

template <typename T>
inline constexpr std::true_type operator != (const T&, const none_t&) { return {};}

template <typename T>
using is_none = decltype(std::declval<const T&>() == none);

template <typename T>
inline constexpr auto IsNone = is_none<std::decay_t<T>>::value;

} // namespace ozo
