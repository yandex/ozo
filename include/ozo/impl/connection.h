#pragma once

#include <ozo/native_conn_handle.h>

#include <ozo/asio.h>
#include <boost/algorithm/string/trim.hpp>
#include <string>
#include <sstream>


namespace ozo {

namespace impl {

template <typename OidMap, typename Statistics>
struct connection_impl {
    connection_impl(io_context& io, Statistics statistics)
    : socket_(io), statistics_(std::move(statistics)) {}

    native_conn_handle handle_;
    asio::posix::stream_descriptor socket_;
    OidMap oid_map_;
    Statistics statistics_; // statistics metatypes to be defined - counter, duration, whatever?
    std::string error_context_;
};

inline bool connection_status_bad(PGconn* handle) noexcept {
    return !handle || PQstatus(handle) == CONNECTION_BAD;
}

template <typename NativeHandleType>
inline auto connection_error_message(NativeHandleType handle) {
    std::string_view v(PQerrorMessage(handle));
    auto trim_pos = v.find_last_not_of(' ');
    if (trim_pos == v.npos) {
        v.remove_suffix(v.size());
    } else if (trim_pos < v.size()) {
        v.remove_suffix(v.size() - trim_pos - 1);
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
