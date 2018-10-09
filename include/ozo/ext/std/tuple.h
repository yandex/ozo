#pragma once

#include <ozo/type_traits.h>
#include <tuple>

namespace ozo::definitions {

template <typename ...Ts>
struct type<std::tuple<Ts...>> :
    detail::type_definition<decltype("record"_s), oid_constant<RECORDOID>>{};

template <typename ...Ts>
struct array<std::tuple<Ts...>> :
    detail::array_definition<std::tuple<Ts...>, oid_constant<RECORDARRAYOID>>{};

} // namespace ozo::definitions

namespace ozo {

template <typename ...Ts>
struct is_composite<std::tuple<Ts...>> : std::true_type {};

} // namespace ozo
