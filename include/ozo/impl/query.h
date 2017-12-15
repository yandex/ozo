#pragma once

#include <ozo/type_traits.h>

#include <boost/hana/tuple.hpp>

#include <string_view>

namespace ozo::impl {

template <class ... ParamsT>
struct query {
    std::string_view text;
    hana::tuple<ParamsT ...> params;
};

template <class ... ParamsT>
const auto& get_query_text(const query<ParamsT ...>& value) {
    return value.text;
}

template <class ... ParamsT>
const auto& get_query_params(const query<ParamsT ...>& value) {
    return value.params;
}

} // namespace ozo::impl
