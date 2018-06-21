#pragma once

#include <ozo/impl/query.h>
#include <ozo/concept.h>

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

template <class, class = std::void_t<>>
struct is_query : std::false_type {};

template <class T>
struct is_query<T, std::void_t<
    decltype(get_query_text(std::declval<const T&>())),
    decltype(get_query_params(std::declval<const T&>()))
>> : std::integral_constant<bool,
    QueryText<decltype(get_query_text(std::declval<const T&>()))>
    && HanaTuple<decltype(get_query_params(std::declval<const T&>()))>
> {};

template <typename T>
constexpr auto Query = is_query<std::decay_t<T>>::value;

template <class Text, class ... ParamsT>
auto make_query(Text&& text, ParamsT&& ... params) {
    static_assert(QueryText<Text>, "text must be QueryText concept");
    using result = impl::query<std::decay_t<Text>, std::decay_t<ParamsT> ...>;
    return result {std::forward<Text>(text), hana::make_tuple(std::forward<ParamsT>(params) ...)};
}

template <class ... ParamsT>
auto make_query(const char* text, ParamsT&& ... params) {
    const auto text_view = std::string_view(text);
    using result = impl::query<std::decay_t<decltype(text_view)>, std::decay_t<ParamsT> ...>;
    return result {text_view, hana::make_tuple(std::forward<ParamsT>(params) ...)};
}

template <class T, class = Require<Query<T>>>
decltype(auto) get_text(const T& query) {
    using impl::get_query_text;
    return get_query_text(query);
}

template <class T, class = Require<Query<T>>>
decltype(auto) get_params(const T& query) {
    using impl::get_query_params;
    return get_query_params(query);
}

} // namespace ozo
