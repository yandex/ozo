#pragma once

#include <ozo/concept.h>
#include <ozo/type_traits.h>
#include <ozo/detail/endian.h>
#include <ozo/detail/float.h>

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
        (std::int32_t, index)
    );
};

template <class T, class OutIteratorT>
constexpr Require<std::is_integral_v<T>, OutIteratorT> write(T value, OutIteratorT out) {
    hana::for_each(
        hana::to<hana::tuple_tag>(hana::make_range(hana::size_c<0>, hana::size_c<sizeof(T)>)),
        [&] (auto i) { *out++ = value >> decltype(i)::value * CHAR_BIT & std::numeric_limits<std::uint8_t>::max(); }
    );
    return out;
}

template <class T, class OutIteratorT>
constexpr Require<std::is_floating_point_v<T>, OutIteratorT> write(T value, OutIteratorT out) {
    return write(detail::convert_to_big_endian(to_integral(value)), out);
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

template <typename T>
constexpr auto SingleByteIntegral = std::is_integral_v<T> && sizeof(T) == 1;

template <class T, class OidMapT, class OutIteratorT>
constexpr Require<SingleByteIntegral<T>, OutIteratorT> send(T value, const OidMapT&, OutIteratorT out) {
    return detail::write(value, out);
}

template <typename T>
constexpr auto MultiByteIntegral = std::is_integral_v<T> && (sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8);

template <class T, class OidMapT, class OutIteratorT>
constexpr Require<MultiByteIntegral<T>, OutIteratorT> send(T value, const OidMapT&, OutIteratorT out) {
    return detail::write(detail::convert_to_big_endian(value), out);
}

template <typename T>
constexpr auto FloatingPoint = std::is_floating_point_v<T> && (sizeof(T) == 4 || sizeof(T) == 8);

template <class T, class OidMapT, class OutIteratorT>
constexpr Require<FloatingPoint<T>, OutIteratorT> send(T value, const OidMapT&, OutIteratorT out) {
    return detail::write(value, out);
}

template <class OidMapT, class OutIteratorT, class CharT, class TraitsT, class AllocatorT>
constexpr OutIteratorT send(const std::basic_string<CharT, TraitsT, AllocatorT>& value, const OidMapT&, OutIteratorT out) {
    boost::for_each(value, detail::make_writer(out));
    return out;
}

template <class OidMapT, class OutIteratorT, class T, class AllocatorT>
constexpr OutIteratorT send(const std::vector<T, AllocatorT>& value, const OidMapT& oid_map, OutIteratorT out) {
    using value_type = std::decay_t<T>;
    out = send(detail::pg_array {1, 0, ::Oid(type_oid<value_type>(oid_map))}, oid_map, out);
    out = send(detail::pg_array_dimension {std::int32_t(std::size(value)), 0}, oid_map, out);
    boost::for_each(value,
        [&] (const auto& v) {
            out = send(std::int32_t(size_of(v)), oid_map, out);
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
