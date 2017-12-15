#pragma once

#include <typeinfo>

namespace libapq {

/**
* This is requirement simulation type, which is the alias to std::enable_if_t
* It is pretty simple to use it with pseudo-concepts such as OperatorNot below.
*/
template <bool Condition, typename Type = void>
using Require = std::enable_if_t<Condition, Type>;


template <typename T, typename = std::void_t<>>
struct has_operator_not : std::false_type {};
template <typename T>
struct has_operator_not<T, std::void_t<decltype(!std::declval<T>())>>
    : std::true_type {};

template <typename T>
constexpr auto OperatorNot = has_operator_not<std::decay_t<T>>::value;

} // namespace libapq
