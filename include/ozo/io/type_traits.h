#pragma once

#include <ozo/io/istream.h>
#include <ozo/io/ostream.h>

namespace ozo {

template <typename T, typename = std::void_t<>>
struct has_resize : std::false_type {};

template <typename T>
struct has_resize<T, std::void_t<decltype(std::declval<T>().resize(std::size_t()))>> : std::true_type {};

template <typename T>
inline constexpr auto Resizable = has_resize<T>::value;

template <typename T, typename = std::void_t<>>
struct is_writable : std::false_type {};

template <typename T>
struct is_writable<T, std::void_t<decltype(read(std::declval<istream&>(), std::declval<T>()))>> : std::true_type {};

template <typename T>
inline constexpr auto Writable = is_writable<T>::value;

template <typename T, typename = std::void_t<>>
struct is_readable : std::false_type {};

template <typename T>
struct is_readable<T, std::void_t<decltype(write(std::declval<ostream&>(), std::declval<T>()))>> : std::true_type {};

template <typename T>
inline constexpr auto Readable = is_readable<T>::value;

} // namespace ozo
