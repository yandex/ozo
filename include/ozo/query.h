#pragma once

#include <ozo/detail/functional.h>
#include <ozo/core/concept.h>
#include <ozo/type_traits.h>

/**
 * @defgroup group-query Queries
 * @brief Database queries related concepts, types and functions.
 */
namespace ozo {

template <typename T>
inline auto to_const_char(const T& v) noexcept -> Require<HanaString<T>, const char*> {
    return hana::to<const char*>(v);
}

template <typename T>
inline auto to_const_char(const T& v) noexcept -> decltype(std::data(v)) {
    return std::data(v);
}

inline auto to_const_char(const char* v) noexcept {
    return v;
}

template <class, class = std::void_t<>>
struct is_query_text : std::false_type {};

template <class T>
struct is_query_text<T, std::void_t<
    decltype(to_const_char(std::declval<const T&>()))
>> : std::integral_constant<bool,
    std::is_same_v<decltype(to_const_char(std::declval<const T&>())), const char*>
    && noexcept(to_const_char(std::declval<const T&>()))
> {};

template <typename T>
constexpr auto QueryText = is_query_text<std::decay_t<T>>::value;

template <typename T>
struct get_query_text_impl;

template <typename T>
inline detail::result_of<get_query_text_impl, T> get_query_text(T&& query) {
    return detail::apply<get_query_text_impl>(std::forward<T>(query));
}

template <typename T>
struct get_query_params_impl;

template <typename T>
inline detail::result_of<get_query_params_impl, T> get_query_params(T&& query) {
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
