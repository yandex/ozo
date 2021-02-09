#pragma once

#include <ozo/error.h>
#include <ozo/pg/handle.h>
#include <boost/algorithm/string/trim.hpp>
#include <boost/asio/steady_timer.hpp>

#include <string>
#include <sstream>

namespace ozo {

namespace detail {

template <typename NativeHandle>
inline bool connection_status_bad(NativeHandle handle) noexcept {
    return !handle || PQstatus(handle) == CONNECTION_BAD;
}

template <typename NativeHandle>
inline bool connection_status_ok(NativeHandle handle) noexcept {
    return handle && PQstatus(handle) == CONNECTION_OK;
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

} // namespace detail

template <typename OidMap, typename Statistics>
connection<OidMap, Statistics>::connection(io_context& io, Statistics statistics)
: io_(std::addressof(io)), socket_(*io_), statistics_(std::move(statistics)) {}

template <typename OidMap, typename Statistics>
error_code connection<OidMap, Statistics>::assign(ozo::pg::conn&& handle) {
    int fd = PQsocket(handle.get());
    if (fd == -1) {
        return error::pq_socket_failed;
    }

    stream_type new_socket(*io_, fd);
    socket_.release();

    socket_ = std::move(new_socket);
    handle_ = std::move(handle);
    return {};
}

template <typename OidMap, typename Statistics>
ozo::pg::conn connection<OidMap, Statistics>::release() {
    socket_.release();
    ozo::pg::conn retval;
    using std::swap;
    swap(retval, handle_);
    return retval;
}

template <typename OidMap, typename Statistics>
template <typename WaitHandler>
void connection<OidMap, Statistics>::async_wait_write(WaitHandler&& h) {
    socket_.async_write_some(asio::null_buffers(), std::forward<WaitHandler>(h));
}

template <typename OidMap, typename Statistics>
template <typename WaitHandler>
void connection<OidMap, Statistics>::async_wait_read(WaitHandler&& h) {
    socket_.async_read_some(asio::null_buffers(), std::forward<WaitHandler>(h));
}

template <typename OidMap, typename Statistics>
error_code connection<OidMap, Statistics>::close() noexcept {
    release().reset();
    return error_code{};
}

template <typename OidMap, typename Statistics>
void connection<OidMap, Statistics>::cancel() noexcept {
    error_code _;
    socket_.cancel(_);
}

template <typename OidMap, typename Statistics>
bool connection<OidMap, Statistics>::is_bad() const noexcept {
    return !detail::connection_status_ok(native_handle());
}

template <typename OidMap, typename Statistics>
connection<OidMap, Statistics>::~connection() {
    close();
}

template <typename Connection>
inline std::string_view error_message(const Connection& conn) {
    static_assert(ozo::Connection<Connection>, "conn should model Connection");
    if (is_null_recursive(conn)) {
        return {};
    }
    return detail::connection_error_message(get_native_handle(conn));
}

template <typename Connection>
inline error_code close_connection(Connection&& conn) {
    static_assert(ozo::Connection<Connection>, "conn should model Connection");
    return unwrap_connection(conn).close();
}

template <typename Connection>
inline bool connection_bad(const Connection& conn) noexcept {
    static_assert(ozo::Connection<Connection>, "conn should model Connection");
    return is_null_recursive(conn) ? true : unwrap_connection(conn).is_bad();
}

template <typename Connection>
inline auto get_native_handle(const Connection& conn) noexcept {
    static_assert(ozo::Connection<Connection>, "conn should model Connection");
    return unwrap_connection(conn).native_handle();
}

template <typename Connection>
inline const auto& get_error_context(const Connection& conn) {
    static_assert(ozo::Connection<Connection>, "conn should model Connection");
    return unwrap_connection(conn).get_error_context();
}

template <typename Connection>
inline auto get_executor(const Connection& conn) noexcept {
    static_assert(ozo::Connection<Connection>, "conn should model Connection");
    return unwrap_connection(conn).get_executor();
}

namespace detail {
inline constexpr std::string_view make_string_view(const char* src) {
    return src == nullptr ? std::string_view{} : std::string_view{src};
}
} // namespace detail

template <typename Connection>
inline std::string_view get_database(const Connection& conn) {
    static_assert(ozo::Connection<Connection>, "conn should model Connection");
    return detail::make_string_view(PQdb(get_native_handle(conn)));
}

template <typename Connection>
inline std::string_view get_host(const Connection& conn) {
    static_assert(ozo::Connection<Connection>, "conn should model Connection");
    return detail::make_string_view(PQhost(get_native_handle(conn)));
}

template <typename Connection>
inline std::string_view get_port(const Connection& conn) {
    static_assert(ozo::Connection<Connection>, "conn should model Connection");
    return detail::make_string_view(PQport(get_native_handle(conn)));
}

template <typename Connection>
inline std::string_view get_user(const Connection& conn) {
    static_assert(ozo::Connection<Connection>, "conn should model Connection");
    return detail::make_string_view(PQuser(get_native_handle(conn)));
}

template <typename Connection>
inline std::string_view get_password(const Connection& conn) {
    static_assert(ozo::Connection<Connection>, "conn should model Connection");
    return detail::make_string_view(PQpass(get_native_handle(conn)));
}

} // namespace ozo
