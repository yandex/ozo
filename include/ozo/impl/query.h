#pragma once

#include <ozo/type_traits.h>

#include <boost/hana/tuple.hpp>

#include <string_view>

namespace ozo::impl {

template <class Text, class ... ParamsT>
struct query {
    Text text;
    hana::tuple<ParamsT ...> params;
};

template <class ... Ts>
const auto& get_query_text(const query<Ts ...>& value) {
    return value.text;
}

template <class ... Ts>
const auto& get_query_params(const query<Ts ...>& value) {
    return value.params;
}

} // namespace ozo::impl
