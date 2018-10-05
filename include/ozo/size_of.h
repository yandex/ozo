#pragma once

#include <ozo/type_traits.h>

namespace ozo {

/**
 * @brief `ozo::size_of` implementation functor
 * @ingroup group-type_system-types
 *
 * This template is used to implement object binary representation size calculation
 * including all the meta-information is used for the PostgreSQL binary protocol.
 * By default it provides `static_assert` failure - which means that there is no
 * know algorithm to calculate size. There are default specializations for different
 * common types and concepts, incluiding `BuiltIn`, `Array`, `Composite`, `Nullable`.
 *
 * @tparam T --- type to examine.
 * @tparam <anonymous> --- SFINAE overloading rule.
 *
 * @note This template has to be defined in case of defining custom IO via `ozo::send_impl`
 * and `ozo::recv_impl`.
 *
 * ### Example
 *
 * This is how it can be implemented for the static size type (*exposition only*):
 *
 * @code
template <>
struct size_of_impl<std::nullptr_t> {
    static constexpr auto apply(std::nullptr_t) noexcept {
        return null_state_size;
    }
};
 * @endcode
 */
template <typename T, typename = std::void_t<>>
struct size_of_impl;

namespace detail {

template <typename T, typename = std::void_t<>>
struct size_of_default_impl {
    static constexpr auto apply(const T&) noexcept {
        return typename type_traits<T>::size{};
    }
};

template <typename T>
struct size_of_default_impl<T, Require<DynamicSize<T>>> {
    static constexpr auto apply(const T& v) noexcept(noexcept(std::size(v))){
        return std::size(v) * sizeof(decltype(*v.begin()));
    }
};

template <typename T, typename = std::void_t<>>
struct size_of_impl_dispatcher { using type = size_of_impl<std::decay_t<T>>; };

template <typename T>
using get_size_of_impl = typename size_of_impl_dispatcher<unwrap_type<T>>::type;

} // namespace detail

/**
 * @brief Returns size of object binary representation in bytes.
 * @ingroup group-type_system-functions
 *
 * This function returns binary representation size of the object
 * is used for the PostgreSQL binary protocol.
 *
 * @param v --- object to examine
 * @return `ozo::size_type` --- size of object in bytes if object is
 * not #Nullable or not in null state.
 * @return `ozo::null_state_size` --- for #Nullable object in null state.
 *
 * @note T has to have `ozo::type_traits` specialization.
 *
 * ### Customization point
 *
 * This function can be customized for the type or concept via `ozo::size_of_impl` structure
 * template specialization.
 *
 * @sa ozo::size_of_impl, OZO_PG_DEFINE_TYPE_AND_ARRAY, OZO_PG_DEFINE_CUSTOM_TYPE
 */
template <typename T>
constexpr size_type size_of(const T& v) {
    static_assert(HasDefinition<T>, "the type has not been defined with PostgreSQL type traits");
    return is_null(v) ? null_state_size : detail::get_size_of_impl<T>::apply(unwrap(v));
}

template <typename T, typename>
struct size_of_impl : detail::size_of_default_impl<T> {};

} // namespace ozo
