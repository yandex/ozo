#pragma once

#include <ozo/type_traits.h>

#include <libpq-fe.h>

#include <boost/hana/adapt_struct.hpp>
#include <boost/hana/for_each.hpp>
#include <boost/hana/members.hpp>
#include <boost/hana/tuple.hpp>
#include <boost/range/algorithm/for_each.hpp>

#include <type_traits>

#include <limits.h>

namespace ozo {
namespace detail {

struct pg_array {
    BOOST_HANA_DEFINE_STRUCT(pg_array,
        (std::int32_t, dimensions_count),
        (std::int32_t, dataoffset),
        (::Oid, elemtype)
    );
};

struct pg_array_dimension {
    BOOST_HANA_DEFINE_STRUCT(pg_array_dimension,
        (std::int32_t, size),
        (std::int32_t, index),
        (std::int32_t, begin)
    );
};

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__

template <class T, std::size_t ... N>
constexpr T bytes_order_swap_impl(T value, std::index_sequence<N ...>) {
    return (((value >> N * CHAR_BIT & std::numeric_limits<std::uint8_t>::max()) << (sizeof(T) - 1 - N) * CHAR_BIT) | ...);
}

template <class T, class U = std::make_unsigned_t<T>>
constexpr U bytes_order_swap(T value) {
    return bytes_order_swap_impl<U>(value, std::make_index_sequence<sizeof(T)> {});
}

#else

template <class T>
constexpr T bytes_order_swap(T value) {
    return value;
}

#endif

template <class T, class OutIteratorT>
constexpr std::enable_if_t<std::is_integral_v<T>, OutIteratorT> write(T value, OutIteratorT out) {
    hana::for_each(
        hana::to<hana::tuple_tag>(hana::make_range(hana::size_c<0>, hana::size_c<sizeof(T)>)),
        [&] (auto i) { *out++ = value >> decltype(i)::value * CHAR_BIT & std::numeric_limits<std::uint8_t>::max(); }
    );
    return out;
}

template <class T>
struct floating_point_integral {};

template <>
struct floating_point_integral<float> {
    using type = std::uint32_t;
};

template <>
struct floating_point_integral<double> {
    using type = std::uint64_t;
};

template <class T>
using floating_point_integral_t = typename floating_point_integral<T>::type;

template <class T, class OutIteratorT>
constexpr std::enable_if_t<std::is_floating_point_v<T>, OutIteratorT> write(T value, OutIteratorT out) {
    union {
        T input;
        floating_point_integral_t<T> output;
    } data {value};
    return write(data.output, out);
}

template <class T>
constexpr std::enable_if_t<std::is_arithmetic_v<T>, std::size_t> get_size(const T&) noexcept {
    return sizeof(T);
}

template <class T>
constexpr std::enable_if_t<!std::is_arithmetic_v<T>, std::size_t> get_size(const T& value) noexcept {
    return std::size(value);
}

template <class OutIteratorT>
auto make_writer(OutIteratorT& out) {
    return [&] (auto v) { out = write(v, out); };
}

template <class OidMapT, class OutIteratorT>
auto make_sender(const OidMapT& oid_map, OutIteratorT& out) {
    return [&] (const auto& v) { out = send(v, oid_map, out); };
}

} // namespace detail

template <class T, class R>
using single_byte_integral_t = std::enable_if_t<std::is_integral_v<T> && sizeof(T) == 1, R>;

template <class T, class OidMapT, class OutIteratorT>
constexpr single_byte_integral_t<T, OutIteratorT> send(T value, const OidMapT&, OutIteratorT out) {
    return detail::write(value, out);
}

template <class T, class R>
using multi_byte_integral_t = std::enable_if_t<std::is_integral_v<T> && (sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8), R>;

template <class T, class OidMapT, class OutIteratorT>
constexpr multi_byte_integral_t<T, OutIteratorT> send(T value, const OidMapT&, OutIteratorT out) {
    return detail::write(detail::bytes_order_swap(value), out);
}

template <class T, class R>
using floating_point_t = std::enable_if_t<std::is_floating_point_v<T> && (sizeof(T) == 4 || sizeof(T) == 8), R>;

template <class T, class OidMapT, class OutIteratorT>
constexpr floating_point_t<T, OutIteratorT> send(T value, const OidMapT&, OutIteratorT out) {
    return detail::write(value, out);
}

template <class OidMapT, class OutIteratorT, class CharT, class TraitsT, class AllocatorT>
constexpr OutIteratorT send(const std::basic_string<CharT, TraitsT, AllocatorT>& value, const OidMapT&, OutIteratorT out) {
    boost::for_each(value, detail::make_writer(out));
    return out;
}

template <class OidMapT, class OutIteratorT, class T, class AllocatorT>
constexpr std::enable_if_t<std::is_arithmetic_v<T>, OutIteratorT> send(const std::vector<T, AllocatorT>& value, const OidMapT& oid_map, OutIteratorT out) {
    out = send(detail::pg_array {1, 0, ::Oid(type_oid<std::decay_t<T>>(oid_map))}, oid_map, out);
    out = send(detail::pg_array_dimension {std::int32_t(std::size(value)), 0, 1}, oid_map, out);
    boost::for_each(value,
        [&] (const auto& v) {
            out = send(std::int32_t(detail::get_size(v)), oid_map, out);
            out = send(v, oid_map, out);
        });
    return out;
}

template <class OidMapT, class OutIteratorT, class AllocatorT>
constexpr OutIteratorT send(const std::vector<char, AllocatorT>& value, const OidMapT&, OutIteratorT out) {
    boost::for_each(value, detail::make_writer(out));
    return out;
}

template <class OidMapT, class OutIteratorT>
constexpr OutIteratorT send(const boost::uuids::uuid& value, const OidMapT&, OutIteratorT out) {
    hana::for_each(value, detail::make_writer(out));
    return out;
}

template <class OidMapT, class OutIteratorT>
constexpr OutIteratorT send(const detail::pg_array& value, const OidMapT& oid_map, OutIteratorT out) {
    hana::for_each(hana::members(value), detail::make_sender(oid_map, out));
    return out;
}

template <class OidMapT, class OutIteratorT>
constexpr OutIteratorT send(const detail::pg_array_dimension& value, const OidMapT& oid_map, OutIteratorT out) {
    hana::for_each(hana::members(value), detail::make_sender(oid_map, out));
    return out;
}

} // namespace ozo
