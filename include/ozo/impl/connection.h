#pragma once

#include <libpq-fe.h>

#include <ozo/asio.h>
#include <boost/algorithm/string/trim.hpp>
#include <string>
#include <sstream>


namespace ozo {

namespace impl {

using pg_native_handle_type = PGconn*;

using pg_conn_handle = std::unique_ptr<PGconn, decltype(&PQfinish)>;

template <typename OidMap, typename Statistics>
struct connection {
    connection(io_context& io, Statistics statistics)
    : handle_(nullptr, &PQfinish), socket_(io), statistics_(std::move(statistics)) {}

    pg_conn_handle handle_;
    asio::posix::stream_descriptor socket_;
    OidMap oid_map_;
    Statistics statistics_; // statistics metatypes to be defined - counter, duration, whatever?
    std::string error_context_;
};

template <typename ...Ts>
inline const auto& get_connection_handle(const connection<Ts...>& ctx) noexcept {
    return ctx.handle_;
}

template <typename ...Ts>
inline auto& get_connection_handle(connection<Ts...>& ctx) noexcept {
    return ctx.handle_;
}

inline bool connection_status_bad(pg_native_handle_type handle) noexcept {
    return !handle || PQstatus(handle) == CONNECTION_BAD;
}

template <typename ...Ts>
inline const auto& get_connection_socket(
        const connection<Ts...>& ctx) noexcept {
    return ctx.socket_;
}

template <typename ...Ts>
inline auto& get_connection_socket(
        connection<Ts...>& ctx) noexcept {
    return ctx.socket_;
}

template <typename ...Ts>
inline const auto& get_connection_oid_map(
        const connection<Ts...>& ctx) noexcept {
    return ctx.oid_map_;
}

template <typename ...Ts>
inline auto& get_connection_oid_map(
        connection<Ts...>& ctx) noexcept {
    return ctx.oid_map_;
}

template <typename ...Ts>
inline const auto& get_connection_statistics(
        const connection<Ts...>& ctx) noexcept {
    return ctx.statistics_;
}

template <typename ...Ts>
inline auto& get_connection_statistics(
        connection<Ts...>& ctx) noexcept {
    return ctx.statistics_;
}

template <typename ...Ts>
inline auto& get_connection_error_context(connection<Ts...>& ctx) {
    return ctx.error_context_;
}

template <typename ...Ts>
inline const auto& get_connection_error_context(
        const connection<Ts...>& ctx) {
    return ctx.error_context_;
}

inline auto connection_error_message(pg_native_handle_type handle) {
    std::string_view v(PQerrorMessage(handle));
    auto trim_pos = v.find_last_not_of(' ');
    if(trim_pos != v.npos) {
        v.remove_suffix(v.size() - trim_pos);
    }
    return v;
}

template <typename Connection, typename IoContext>
inline error_code rebind_connection_io_context(Connection& conn, IoContext& io) {
    if (std::addressof(get_io_context(conn)) != std::addressof(io)) {
        decltype(auto) socket = get_socket(conn);
        std::decay_t<decltype(socket)> s{io};
        error_code ec;
        s.assign(socket.native_handle(), ec);
        if (ec) {
            return ec;
        }
        socket.release();
        socket = std::move(s);
    }
    return {};
}

} // namespace impl
} // namespace ozo
