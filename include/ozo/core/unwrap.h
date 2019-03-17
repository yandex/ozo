#pragma once

#include <ozo/core/nullable.h>
#include <ozo/core/concept.h>
#include <ozo/detail/functional.h>

#include <boost/hana/core/when.hpp>

namespace ozo {

namespace hana = boost::hana;

/**
 * @brief Implementation of `ozo::unwrap()`
 *
 * This function defines how to unwrap ano object of type T by means of
 * static member function apply. E.g. in most common cases it may be defined
 * in general way like:
 *
 * @code
template <typename Arg>
constexpr static decltype(auto) apply(Arg&& v);
 * @endcode
 *
 * In case if you want to specify different behaviour for different type of references
 * the function may be implemented in specific way like:
 *
 * @code
constexpr static decltype(auto) apply(T&& v);
constexpr static decltype(auto) apply(T& v);
constexpr static decltype(auto) apply(const T& v);
 * @endcode
 *
 * @tparam T --- decayed type to apply function to.
 * @tparam When --- boost::hana::when specified condition for the overloading.
 * @ingroup group-core-types
 */
template <typename T, typename When = hana::when<true>>
struct unwrap_impl : detail::functional::forward {};

/**
 * @brief Unwraps argument underlying value or forwards the argument.
 *
 * This utility function returns underlying value of it's argument as defined by
 * the operation implementation `ozo::unwrap_impl<std::decay_t<T>>::apply(std::forward<T>(v))`.
 *
 * Default in-library implementations:
 * - for #Nullable - returns result of argument object dereference,
 * - for `std::reference_wrapper` - returns result of `std::reference_wrapper::get()` member function,
 * - in other cases - returns result of `std::forward<T>(v)`
 *
 * @param value --- object to unwrap
 * @return implementation depended value.
 * @ingroup group-core-functions
 */
template <typename T>
constexpr decltype(auto) unwrap(T&& v) noexcept(
        noexcept(unwrap_impl<std::decay_t<T>>::apply(std::forward<T>(v)))) {
    return unwrap_impl<std::decay_t<T>>::apply(std::forward<T>(v));
}

template <typename T>
struct unwrap_impl<std::reference_wrapper<T>> {
    template <typename Ref>
    constexpr static decltype(auto) apply(Ref&& v) noexcept {
        return v.get();
    }
};

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
