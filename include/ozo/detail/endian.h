#pragma once

#include <ozo/concept.h>

#include <limits.h>

namespace ozo::detail {

enum class endian {
#ifdef _WIN32
    little = 0,
    big    = 1,
    native = little
#else
    little = __ORDER_LITTLE_ENDIAN__,
    big    = __ORDER_BIG_ENDIAN__,
    native = __BYTE_ORDER__
#endif
};

template <class T, std::size_t ... N>
constexpr T byte_order_swap(T value, std::index_sequence<N ...>) {
    return (((value >> N * CHAR_BIT & std::numeric_limits<std::uint8_t>::max()) << (sizeof(T) - 1 - N) * CHAR_BIT) | ...);
}

template <class T, class U = std::make_unsigned_t<T>>
constexpr Require<endian::native == endian::little, U> convert_to_big_endian(T value) {
    return byte_order_swap<U>(value, std::make_index_sequence<sizeof(T)> {});
}

template <class T>
constexpr Require<endian::native == endian::big, T> convert_to_big_endian(T value) {
    return value;
}

template <class T, class U = std::make_unsigned_t<T>>
constexpr Require<endian::native == endian::little, U> convert_from_big_endian(T value) {
    return byte_order_swap<U>(value, std::make_index_sequence<sizeof(T)> {});
}

template <class T>
constexpr Require<endian::native == endian::big, T> convert_from_big_endian(T value) {
    return value;
}

}