#pragma once

#include <ozo/detail/functional.h>
#include <ozo/core/concept.h>
#include <ozo/type_traits.h>

/**
 * @defgroup group-query Queries
 * @brief Database queries related concepts, types and functions.
 */
namespace ozo {

template <typename T, typename = hana::when<true>>
struct to_const_char_impl {};

template <typename T>
struct to_const_char_impl<T, hana::when<HanaString<T>>> {
    static constexpr const char* apply(const T& v) noexcept {
        return hana::to<const char*>(v);
    }
};

template <>
struct to_const_char_impl<std::string> {
    static const char* apply(const std::string& v) noexcept {
        return v.data();
    }
};

template <>
struct to_const_char_impl<std::string_view> {
    static constexpr const char* apply(std::string_view v) noexcept {
        return v.data();
    }
};

template <>
struct to_const_char_impl<const char*> {
    static constexpr const char* apply(const char* v) noexcept {
        return v;
    }
};

template <typename T>
inline constexpr detail::result_of<to_const_char_impl, T> to_const_char(const T& v) noexcept {
    static_assert(noexcept(detail::apply<to_const_char_impl>(v)),
        "to_const_char_impl::apply() should be noexcept");
    static_assert(std::is_same_v<const char*, detail::result_of<to_const_char_impl, T>>,
        "to_const_char_impl::apply() should return const char*");
    return detail::apply<to_const_char_impl>(v);
}

template <class, class = std::void_t<>>
struct is_query_text : std::false_type {};

template <class T>
struct is_query_text<T, std::void_t<
    decltype(ozo::to_const_char(std::declval<const T&>()))
>> : std::true_type {};

template <typename T>
constexpr auto QueryText = is_query_text<std::decay_t<T>>::value;

template <typename T>
struct get_query_text_impl;

template <typename T>
inline constexpr detail::result_of<get_query_text_impl, T> get_query_text(T&& query) {
    return detail::apply<get_query_text_impl>(std::forward<T>(query));
}

template <typename T>
struct get_query_params_impl;

template <typename T>
inline constexpr detail::result_of<get_query_params_impl, T> get_query_params(T&& query) {
    return detail::apply<get_query_params_impl>(std::forward<T>(query));
}

template <class, class = std::void_t<>>
struct is_query : std::false_type {};

template <class T>
struct is_query<T, std::void_t<
    decltype(get_query_text(std::declval<const T&>())),
    decltype(get_query_params(std::declval<const T&>()))
>> : std::bool_constant<
    QueryText<decltype(get_query_text(std::declval<const T&>()))>
    && HanaSequence<decltype(get_query_params(std::declval<const T&>()))>
> {};

template <typename T>
constexpr auto Query = is_query<std::decay_t<T>>::value;

template <class Text, class ... ParamsT>
inline constexpr auto make_query(Text&& text, ParamsT&& ... params);

template <class ... ParamsT>
inline constexpr auto make_query(const char* text, ParamsT&& ... params) {
    return make_query(std::string_view(text), std::forward<ParamsT>(params)...);
}

template <class T, class = Require<Query<T>>>
decltype(auto) get_text(const T& query) {
    return get_query_text(query);
}

template <class T, class = Require<Query<T>>>
decltype(auto) get_params(const T& query) {
    return get_query_params(query);
}

} // namespace ozo

#include <ozo/impl/query.h>
