#pragma once

#include <ozo/core/nullable.h>
#include <ozo/core/unwrap.h>

namespace ozo {

template <typename T, typename = hana::when<true>>
struct unwrap_recursive_impl : unwrap_impl<T> {};

/**
 * @brief Unwraps argument underlying value recursively.
 *
 * This utility function unwraps argument recursively applying `ozo::unwrap()` function until
 * the type of unwrapped value and type of it's argument become the same.
 *
 * @note Before applying the function it is better to check the object recursively for a null
 * state via `ozo::is_null_recursive()`.
 *
 * The function is equal to this pesudo-code:
 * @code
constexpr decltype(auto) unwrap_recursive(auto&& arg) noexcept {
    while (decltype(arg) != unwrap_type<decltype(arg)>) {
        arg = unwrap(arg);
    }
    return arg;
}
 * @endcode
 *
 * @param value --- object to unwrap
 * @return implementation depended result of recursively applied `ozo::unwrap()`
 * @ingroup group-core-functions
 * @sa ozo::is_null_recursive()
 */
template <typename T>
inline constexpr decltype(auto) unwrap_recursive(T&& v) noexcept {
    return unwrap_recursive_impl<std::decay_t<T>>::apply(std::forward<T>(v));
}

template <typename T>
struct unwrap_recursive_impl<T, hana::when<!std::is_same_v<T, unwrap_type<T>>>> {
    template <typename TT>
    static constexpr decltype(auto) apply(TT&& v) noexcept {
        return unwrap_recursive(unwrap(v));
    }
};

template <typename T, typename = hana::when<true>>
struct is_null_recursive_impl : is_null_impl<T> {};

/**
 * @brief Indicates if one of unwrapped values is in null state
 *
 * This utility function recursively examines the value for a null state.
 * The function is useful to examine #Connection object for a null state
 * because it is normal for such object to be wrapped.
 *
 * The function is equal to this pesudo-code:
 * @code
constexpr bool is_null_recursive(auto&& arg) noexcept {
    while (decltype(arg) != unwrap_type<decltype(arg)>) {
        if (is_null(arg)) {
            return true;
        }
        arg = unwrap(arg);
    }
    return false;
}
 * @endcode
 *
 * @param v --- object to examine
 * @return `true` --- one of the objects is in null-state,
 * @return `false` --- overwise.
 * @ingroup group-core-functions
 * @sa ozo::is_null(), ozo::unwrap()
 */
template <typename T>
inline constexpr bool is_null_recursive(T&& v) noexcept {
    return is_null_recursive_impl<std::decay_t<T>>::apply(std::forward<T>(v));
}

template <typename T>
struct is_null_recursive_impl<T, hana::when<!std::is_same_v<T, unwrap_type<T>>>> {
    template <typename TT>
    static constexpr bool apply(TT&& v) noexcept {
        return is_null(v) ? true : is_null_recursive(unwrap(v));
    }
};

} // namespace ozo
