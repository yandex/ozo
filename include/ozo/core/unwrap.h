#pragma once

#include <ozo/core/nullable.h>
#include <ozo/core/concept.h>

namespace ozo {

namespace detail {

struct unwrap_with_forward {
    template <typename T>
    constexpr static decltype(auto) apply(T&& v) noexcept {
        return std::forward<T>(v);
    }
};

struct unwrap_with_dereference {
    template <typename T>
    constexpr static decltype(auto) apply(T&& v) noexcept(noexcept(*v)) {
        return *v;
    }
};

struct unwrap_with_get {
    template <typename T>
    constexpr static decltype(auto) apply(T&& v) noexcept(noexcept(v.get())) {
        return v.get();
    }
};

template <typename T, typename = std::void_t<>>
struct unwrap_impl_default : unwrap_with_forward {};

template <typename T>
struct unwrap_impl_default <T, Require<Nullable<T>>> : unwrap_with_dereference {};

} // namespace detail


template <typename T>
struct unwrap_impl : detail::unwrap_impl_default<T> {};

/**
 * @brief Dereference #Nullable argument or forward it
 *
 * This utility function gets value from #Nullable, and pass object as it is if it is not #Nullable
 *
 * @param value --- object to unwrap
 * @return dereferenced argument in case of #Nullable, pass as is if it is not.
 * @ingroup group-core-functions
 */
template <typename T>
constexpr decltype(auto) unwrap(T&& v) noexcept(
        noexcept(unwrap_impl<std::decay_t<T>>::apply(std::forward<T>(v)))) {
    return unwrap_impl<std::decay_t<T>>::apply(std::forward<T>(v));
}

template <typename T>
struct unwrap_impl<std::reference_wrapper<T>> : detail::unwrap_with_get {};

/**
 * @brief Unwraps nullable or reference wrapped type
 * @ingroup group-core-types
 * Sometimes it is needed to know the underlying type of #Nullable or
 * type is wrapped with `std::reference_type`. So that is why the type exists.
 * Convenient shortcut is `ozo::unwrap_type`.
 * @tparam T -- type to unwrap.
 *
 * ### Example
 *
 * @code
int a = 42;
auto ref_a = std::ref(a);
auto opt_a = std::make_optional(a);

static_assert(std::is_same<ozo::unwrap_type<decltype(ref_a)>, decltype(a)>);
static_assert(std::is_same<ozo::unwrap_type<decltype(opt_a)>, decltype(a)>);
 * @endcode
 *
 * @sa ozo::unwrap_type, ozo::unwrap
 */
template <typename T>
struct get_unwrapped_type {
#ifdef OZO_DOCUMENTATION
    using type = <unwrapped type>; //!< Unwrapped type
#else
    using type = std::decay_t<decltype(unwrap(std::declval<T>()))>;
#endif
};

/**
 * @brief Shortcut for `ozo::get_unwrapped_type`.
 * @ingroup group-core-types
 *
 * @tparam T -- type to unwrap.
 */
template <typename T>
using unwrap_type = typename get_unwrapped_type<T>::type;

} // namespace ozo
