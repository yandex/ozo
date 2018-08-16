#pragma once

#include <tuple>
#include <vector>
#include <list>

/**
 * Useful shortcuts for typed unnamed results containers
 */
namespace ozo {

template <typename ... Ts>
using typed_row = std::tuple<Ts...>;

template <typename ... Ts>
using rows_of = std::vector<typed_row<Ts...>>;

template <typename ... Ts>
using lrows_of = std::list<typed_row<Ts...>>;

template <typename T>
constexpr auto into(T& v) -> decltype(std::back_inserter(v)) {
    return std::back_inserter(v);
}

} // namespace ozo
