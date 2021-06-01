#pragma once

#include <ozo/io/send.h>
#include <ozo/io/recv.h>
#include <ozo/io/size_of.h>
#include <ozo/type_traits.h>
#include <ozo/pg/definitions.h>

#include <boost/hana/adapt_struct.hpp>
#include <boost/asio/ip/network_v4.hpp>
#include <boost/asio/ip/network_v6.hpp>

#include <variant>
#include <cstdint>

/**
 * @defgroup group-ext-boost-asio-ip boost::asio::ip
 * @ingroup group-ext-boost
 * @brief [boost::asio::ip::network_v4](https://www.boost.org/doc/libs/1_66_0/doc/html/boost_asio/reference/ip__network_v4.html),
 * [boost::asio::ip::network_v6](https://www.boost.org/doc/libs/1_66_0/doc/html/boost_asio/reference/ip__network_v6.html),
 * [boost::asio::ip::address_v4](https://www.boost.org/doc/libs/1_66_0/doc/html/boost_asio/reference/ip__address_v4.html),
 * [boost::asio::ip::address_v6](https://www.boost.org/doc/libs/1_66_0/doc/html/boost_asio/reference/ip__address_v6.html) support
 *
 *@code
#include <ozo/ext/boost/asio_ip.h>
 *@endcode
 *
 * These types are mapped as `inet` PostgreSQL type. If data brings unexpected type the `system_error`
 * with `error::bad_inet_type` error code would be thrown:
 * * `boost::asio::ip::network_v4` is mapped as `inet` PostgreSQL type with family=2(v4) and cidr=1,
 * * `boost::asio::ip::network_v6` is mapped as `inet` PostgreSQL type with family=3(v6) and cidr=1,
 * * `boost::asio::ip::address_v4` is mapped as `inet` PostgreSQL type with family=2(v4) and cidr=0,
 * * `boost::asio::ip::address_v6` is mapped as `inet` PostgreSQL type with family=3(v6) and cidr=1.
 *
 * To store any of the possible address types the `std::variant<network_v4, network_v6, address_v4, address_v6>`
 * is mapped as `inet` PostgreSQL type. This is the recommended way to receive `inet` type.
 */

namespace ozo::detail {

struct pg_inet_header {
    BOOST_HANA_DEFINE_STRUCT(pg_inet_header,
        (std::uint8_t, family),
        (std::uint8_t, netmask),
        (std::uint8_t, cidr),
        (std::uint8_t, addrlen)
    );
};

using network_v4 = boost::asio::ip::network_v4;
using network_v6 = boost::asio::ip::network_v6;
using address_v4 = boost::asio::ip::address_v4;
using address_v6 = boost::asio::ip::address_v6;

template <typename T>
struct pg_inet_traits;

template <>
struct pg_inet_traits<address_v4> {
    static constexpr auto family = 2;
    static constexpr auto max_prefix_len = 32;
};

template <>
struct pg_inet_traits<address_v6> {
    static constexpr auto family = 3;
    static constexpr auto max_prefix_len = 128;
};

template <typename T>
struct asio_inet_traits;

template <typename T>
struct asio_network_traits {
    using type = T;
    using address_type = decltype(std::declval<const T&>().address());
    using bytes_type = typename address_type::bytes_type;

    static constexpr auto family = pg_inet_traits<address_type>::family;
    static constexpr auto addrlen = bytes_type{}.size();
    static constexpr auto cidr = 1;

    static auto construct(bytes_type bytes, const pg_inet_header& header) {
        return type{address_type{bytes}, header.netmask};
    }
    static auto prefix_length(const type& x) { return x.prefix_length(); }
    static bytes_type to_bytes(const type& x) { return x.address().to_bytes(); }
};

template <typename T>
struct asio_address_traits {
    using type = T;
    using address_type = type;
    using bytes_type = typename address_type::bytes_type;

    static constexpr auto family = pg_inet_traits<address_type>::family;
    static constexpr auto addrlen = bytes_type{}.size();
    static constexpr auto cidr = 0;

    static auto construct(bytes_type bytes, const pg_inet_header&) { return type{bytes};}
    static auto prefix_length(const type&) {
        return pg_inet_traits<address_type>::max_prefix_len;
    }
    static bytes_type to_bytes(const type& x) { return x.to_bytes();}
};

template <>
struct asio_inet_traits<network_v4> : asio_network_traits<network_v4> {};
template <>
struct asio_inet_traits<network_v6> : asio_network_traits<network_v6> {};
template <>
struct asio_inet_traits<address_v4> : asio_address_traits<address_v4> {};
template <>
struct asio_inet_traits<address_v6> : asio_address_traits<address_v6> {};

struct send_asio_address {
    template <typename OidMap, typename In>
    static ostream& apply(ostream& out, const OidMap&, const In& in) {
        static_assert(ozo::OidMap<OidMap>, "OidMap should model ozo::OidMap");

        using traits = asio_inet_traits<In>;

        detail::pg_inet_header header;
        header.family = traits::family;
        header.netmask = traits::prefix_length(in);
        header.cidr = traits::cidr;
        header.addrlen = traits::addrlen;

        write(out, header);
        write(out, traits::to_bytes(in));
        return out;
    }
};

struct send_asio_inet {
    template <typename OidMap, typename ...Ts>
    static ostream& apply(ostream& out, const OidMap& map, const std::variant<Ts...>& in) {
        std::visit([&](auto& x) { send_asio_address::apply(out, map, x);}, in);
    }
};

template <typename Traits>
constexpr auto read_asio_address = [](istream& in, const pg_inet_header& header) {
    if (header.addrlen != Traits::addrlen) {
        throw system_error(error::bad_object_size,
            "received address length " + std::to_string(header.addrlen)
            + " does not match expected " + std::to_string(Traits::addrlen));
    }

    typename Traits::bytes_type bytes;
    read(in, bytes);
    return Traits::construct(bytes, header);
};

struct recv_asio_address {
    template <typename OidMap, typename Out>
    static istream& apply(istream& in, size_type, const OidMap&, Out& out) {
        static_assert(ozo::OidMap<OidMap>, "OidMap should model ozo::OidMap");

        detail::pg_inet_header header;
        read(in, header);

        using traits = asio_inet_traits<Out>;

        if (header.cidr != traits::cidr) {
            throw system_error(error::bad_inet_type,
                "received cidr " + std::to_string(header.cidr)
                + " does not match expected " + std::to_string(traits::cidr));
        }

        if (header.family != traits::family) {
            throw system_error(error::bad_inet_type,
                "received family " + std::to_string(header.family)
                + " does not match expected " + std::to_string(traits::family));
        }

        out = read_asio_address<traits>(in, header);
        return in;
    }
};

struct recv_asio_inet {
    template <typename OidMap, typename ...Ts>
    static istream& apply(istream& in, size_type, const OidMap&, std::variant<Ts...>& out) {
        static_assert(ozo::OidMap<OidMap>, "OidMap should model ozo::OidMap");

        detail::pg_inet_header header;
        read(in, header);

        auto processed = false;
        constexpr auto traits = hana::make_basic_tuple(asio_inet_traits<Ts>{}...);
        hana::for_each(traits, [&](auto x) {
            using T = decltype(x);
            if (!processed && header.cidr == T::cidr && header.family == T::family) {
                out = read_asio_address<T>(in, header);
                processed = true;
            }
        });

        if (!processed) {
            throw system_error(error::bad_result_process,
                "no type has been found for inet.family=" + std::to_string(header.family)
                + " inet.cidr=" + std::to_string(header.cidr));
        }

        return in;
    }
};

struct size_of_asio_address {
    template <typename T>
    static constexpr auto apply(const T&) noexcept {
        constexpr const auto header_size = hana::unpack(
            hana::members(detail::pg_inet_header{}),
            [] (const auto& ...x) { return (sizeof(x) + ... + 0); });
        return size_constant<header_size + detail::asio_inet_traits<T>::addrlen>{};
    }
};

} // namespace ozo::detail

OZO_PG_BIND_TYPE(boost::asio::ip::network_v4, "inet")
OZO_PG_BIND_TYPE(boost::asio::ip::network_v6, "inet")
OZO_PG_BIND_TYPE(boost::asio::ip::address_v4, "inet")
OZO_PG_BIND_TYPE(boost::asio::ip::address_v6, "inet")

namespace ozo {

using asio_inet = std::variant<
    boost::asio::ip::network_v4,
    boost::asio::ip::network_v6,
    boost::asio::ip::address_v4,
    boost::asio::ip::address_v6>;

template <>
struct send_impl<boost::asio::ip::network_v4> : detail::send_asio_address {};
template <>
struct send_impl<boost::asio::ip::network_v6> : detail::send_asio_address {};
template <>
struct send_impl<boost::asio::ip::address_v4> : detail::send_asio_address {};
template <>
struct send_impl<boost::asio::ip::address_v6> : detail::send_asio_address {};
template <>
struct send_impl<asio_inet> : detail::send_asio_inet {};

template <>
struct recv_impl<boost::asio::ip::network_v4> : detail::recv_asio_address {};
template <>
struct recv_impl<boost::asio::ip::network_v6> : detail::recv_asio_address {};
template <>
struct recv_impl<boost::asio::ip::address_v4> : detail::recv_asio_address {};
template <>
struct recv_impl<boost::asio::ip::address_v6> : detail::recv_asio_address {};
template <>
struct recv_impl<asio_inet> : detail::recv_asio_inet {};

template <>
struct size_of_impl<boost::asio::ip::network_v4> : detail::size_of_asio_address {};
template <>
struct size_of_impl<boost::asio::ip::network_v6> : detail::size_of_asio_address {};
template <>
struct size_of_impl<boost::asio::ip::address_v4> : detail::size_of_asio_address {};
template <>
struct size_of_impl<boost::asio::ip::address_v6> : detail::size_of_asio_address {};
template <>
struct size_of_impl<asio_inet> {
    static size_type apply(const asio_inet& v) noexcept {
        return std::visit([](const auto& x) { return size_of(x);}, v);
    }
};

} // namespace ozo

OZO_PG_BIND_TYPE(ozo::asio_inet, "inet")
