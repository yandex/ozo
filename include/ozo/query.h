#pragma once

#include <ozo/impl/query.h>
#include <ozo/concept.h>

namespace ozo {

template <class, class = std::void_t<>>
struct is_query : std::false_type {};

template <class T>
struct is_query<T, std::void_t<
    decltype(get_query_text(std::declval<const T&>())),
    decltype(get_query_params(std::declval<const T&>()))
>> : std::true_type {};

template <typename T>
constexpr auto Query = is_query<std::decay_t<T>>::value;

template <class ... ParamsT>
auto make_query(std::string_view text, ParamsT&& ... params) {
    using result = impl::query<std::decay_t<ParamsT> ...>;
    return result {text, hana::make_tuple(std::forward<ParamsT>(params) ...)};
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
