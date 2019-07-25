#pragma once

#include <ozo/native_conn_handle.h>
#include <ozo/asio.h>

#include <boost/algorithm/string/trim.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>

#include <string>
#include <sstream>

namespace ozo {

namespace impl {

template <typename OidMap, typename Statistics>
struct connection_impl {
    connection_impl(io_context& io, Statistics statistics)
        : io_(std::addressof(io)), socket_(*io_), statistics_(std::move(statistics)) {}

    native_conn_handle handle_;
    io_context* io_;
    asio::posix::stream_descriptor socket_;
    OidMap oid_map_;
    Statistics statistics_; // statistics metatypes to be defined - counter, duration, whatever?
    std::string error_context_;

    auto get_executor() const { return io_->get_executor(); }
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

template <typename Connection, typename Executor>
inline error_code bind_connection_executor(Connection& conn, const Executor& ex) {
    if (get_executor(conn) != ex) {
        decltype(auto) socket = get_socket(conn);
        std::decay_t<decltype(socket)> s{ex.context()};
        error_code ec;
        s.assign(socket.native_handle(), ec);
        if (ec) {
            return ec;
        }
        socket.release();
        socket = std::move(s);
        conn.io_ = std::addressof(ex.context());
    }
    return {};
}

} // namespace impl
} // namespace ozo
