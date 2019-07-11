#pragma once

#include <ozo/query.h>
#include <ozo/type_traits.h>

#include <boost/hana/fold.hpp>
#include <boost/hana/for_each.hpp>
#include <boost/hana/string.hpp>
#include <boost/hana/tuple.hpp>
#include <boost/hana/prepend.hpp>

namespace ozo {
namespace detail {

template <std::size_t value>
constexpr auto digit_to_string(hana::size_t<value>) noexcept {
    return hana::string_c<'0' + value>;
}

template <std::size_t value>
constexpr auto to_string(hana::size_t<value> v) noexcept;

template <std::size_t value>
constexpr auto to_string_head(hana::size_t<value> v) noexcept {
    return to_string(v);
}

constexpr auto to_string_head(hana::size_t<0>) noexcept {
    return hana::string_c<>;
}

template <std::size_t value>
constexpr auto to_string(hana::size_t<value>) noexcept {
    constexpr auto divident = value % 10;
    return to_string_head(hana::size_c<(value - divident) / 10>) + digit_to_string(hana::size_c<divident>);
}

template <std::size_t value>
constexpr auto incremented(hana::size_t<value>) {
    return hana::size_c<value + 1>;
}

struct nothing_t {};

template <class T>
constexpr decltype(auto) operator +(nothing_t, T&& value) {
    return std::forward<T>(value);
}

template <class T>
constexpr decltype(auto) operator +(T&& value, nothing_t) {
    return std::forward<T>(value);
}

template <char ... lhs_c, char ... rhs_c>
constexpr auto concat(hana::string<lhs_c ...> lhs, hana::string<rhs_c ...> rhs) {
    return lhs + rhs;
}

template <char ... rhs_c>
constexpr auto concat(const std::string& lhs, hana::string<rhs_c ...> rhs) {
    return lhs + hana::to<const char*>(rhs);
}

} // namespace detail

struct query_text_tag {};

struct query_param_tag {};

template <class ValueT, class TagT>
struct query_element {
    using tag_type = TagT;

    ValueT value;
};

template <class ElementsT>
struct query_builder {
    using elements_type = ElementsT;

    elements_type elements;

    constexpr auto text() const noexcept {
        using namespace detail;
        using namespace hana::literals;
        return hana::fold(elements, hana::tuple<hana::size_t<1>, nothing_t>{},
            [] (auto r, const auto& e) {
                using tag_type = typename std::decay_t<decltype(e)>::tag_type;
                if constexpr (std::is_same_v<tag_type, query_text_tag>) {
                    return hana::make_tuple(r[0_c], r[1_c] + std::move(e.value));
                } else if constexpr (std::is_same_v<tag_type, query_param_tag>) {
                    return hana::make_tuple(incremented(r[0_c]), detail::concat(r[1_c], "$"_s + to_string(r[0_c])));
                } else {
                    static_assert(std::is_same_v<tag_type, void>, "unsupported tag type");
                }
            })[1_c];
    }

    constexpr auto params() const noexcept {
        using namespace detail;
        return hana::fold(elements, hana::tuple<>(),
            [] (auto&& r, auto&& e) {
                using tag_type = typename std::decay_t<decltype(e)>::tag_type;
                if constexpr (std::is_same_v<tag_type, query_text_tag>) {
                    return std::move(r);
                } else if constexpr (std::is_same_v<tag_type, query_param_tag>) {
                    return hana::append(std::move(r), std::move(e.value));
                } else {
                    static_assert(std::is_same_v<tag_type, void>, "unsupported tag type");
                }
            });
    }

    constexpr auto build() const {
        return hana::unpack(
            params(),
            [&] (auto&& ... params) {
                return make_query(text(), std::move(params) ...);
            }
        );
    }
};

template <class T>
struct is_query_builder : std::false_type {};

template <class T>
struct is_query_builder<query_builder<T>> : std::true_type {};

template <class T>
constexpr auto QueryBuilder = is_query_builder<std::decay_t<T>>::value;

template <class T>
constexpr auto make_query_text(T&& value) {
    return query_element<std::decay_t<T>, query_text_tag> {std::forward<T>(value)};
}

template <class T>
constexpr auto make_query_param(T&& value) {
    return query_element<std::decay_t<T>, query_param_tag> {std::forward<T>(value)};
}

template <class ... ValueT, class ... TagT>
constexpr auto make_query_builder(hana::tuple<query_element<ValueT, TagT> ...>&& elements) {
    return query_builder<hana::tuple<query_element<ValueT, TagT> ...>> {std::move(elements)};
}

template <class LhsElementsT, class RhsElementsT>
constexpr auto operator +(query_builder<LhsElementsT>&& lhs, query_builder<RhsElementsT>&& rhs) {
    return make_query_builder(hana::concat(std::move(lhs.elements), std::move(rhs.elements)));
}

template <class LhsElementsT, class RhsValueT, class RhsTagT>
constexpr auto operator +(query_builder<LhsElementsT>&& builder, query_element<RhsValueT, RhsTagT>&& element) {
    return make_query_builder(hana::append(std::move(builder.elements), std::move(element)));
}

template <class LhsValueT, class LhsTagT, class RhsElementsT>
constexpr auto operator +(query_element<LhsValueT, LhsTagT>&& lhs, query_builder<RhsElementsT>&& rhs) {
    return make_query_builder(hana::prepend(std::move(lhs), std::move(rhs.elements)));
}

template <class LhsElementsT, class RhsValueT>
constexpr auto operator +(query_builder<LhsElementsT>&& lhs, RhsValueT&& rhs) {
    return std::move(lhs) + make_query_param(std::forward<RhsValueT>(rhs));
}

template <class LhsValueT, class LhsTagT, class RhsValueT, class RhsTagT>
constexpr auto operator +(query_element<LhsValueT, LhsTagT>&& lhs, query_element<RhsValueT, RhsTagT>&& rhs) {
    return make_query_builder(hana::make_tuple(std::move(lhs), std::move(rhs)));
}

template <class LhsValueT, class LhsTagT, class RhsValueT>
constexpr auto operator +(query_element<LhsValueT, LhsTagT>&& lhs, RhsValueT&& rhs) {
    return make_query_builder(hana::make_tuple(std::move(lhs), make_query_param(std::forward<RhsValueT>(rhs))));
}


namespace literals {

#ifdef __GNUG__

template <class CharT, CharT ... c>
constexpr auto operator "" _SQL() {
    return make_query_builder(hana::make_tuple(make_query_text(hana::string<c ...>())));
}

#endif /* __GNUG__ */

} // namespace literals
} // namespace ozo
