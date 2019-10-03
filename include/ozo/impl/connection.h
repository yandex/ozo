#pragma once

#include <ozo/error.h>
#include <ozo/native_conn_handle.h>
#include <boost/algorithm/string/trim.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>

#include <string>
#include <sstream>

namespace ozo {

namespace impl {

template <typename Connection, typename Executor>
inline error_code bind_connection_executor(Connection& conn, const Executor& ex) {
    if (conn.get_executor() != ex) {
        typename Connection::stream_type s{ex.context()};
        error_code ec;
        s.assign(conn.socket_.native_handle(), ec);
        if (ec) {
            return ec;
        }
        conn.socket_.release();
        conn.socket_ = std::move(s);
        conn.io_ = std::addressof(ex.context());
    }
    return {};
}

template <typename OidMap, typename Statistics>
struct connection_impl {
    connection_impl(io_context& io, Statistics statistics)
        : io_(std::addressof(io)), socket_(*io_), statistics_(std::move(statistics)) {}

    using stream_type = asio::posix::stream_descriptor;
    using native_handle_type = native_conn_handle::pointer;
    using oid_map_type = OidMap;
    using error_context = std::string;
    using executor_type = io_context::executor_type;

    native_conn_handle handle_;
    io_context* io_;
    stream_type socket_;
    oid_map_type oid_map_;
    Statistics statistics_; // statistics metatypes to be defined - counter, duration, whatever?
    error_context error_context_;

    native_handle_type native_handle() const noexcept { return handle_.get(); }

    oid_map_type& oid_map() noexcept { return oid_map_;}
    const oid_map_type& oid_map() const noexcept { return oid_map_;}

    const error_context& get_error_context() const noexcept { return error_context_; }
    void set_error_context(error_context v = error_context{}) { error_context_ = std::move(v); }

    executor_type get_executor() const noexcept { return io_->get_executor(); }

    template <typename Executor>
    error_code bind_executor(const Executor& ex) {
        return ozo::impl::bind_connection_executor(*this, ex);
    }

    error_code assign(native_conn_handle&& handle) {
        int fd = PQsocket(handle.get());
        if (fd == -1) {
            return error::pq_socket_failed;
        }

        int new_fd = dup(fd);
        if (new_fd == -1) {
            set_error_context("error while dup(fd) for socket stream");
            return error_code{errno, boost::system::generic_category()};
        }

        error_code ec;
        socket_.assign(new_fd, ec);

        if (ec) {
            set_error_context("assign socket failed");
            return ec;
        }

        handle_ = std::move(handle);
        return ec;
    }

    template <typename WaitHandler>
    void async_wait_write(WaitHandler&& h) {
        socket_.async_write_some(asio::null_buffers(), std::forward<WaitHandler>(h));
    }

    template <typename WaitHandler>
    void async_wait_read(WaitHandler&& h) {
        socket_.async_read_some(asio::null_buffers(), std::forward<WaitHandler>(h));
    }

    error_code close() noexcept {
        error_code ec;
        socket_.close(ec);
        handle_.reset();
        return ec;
    }

    void cancel() noexcept {
        error_code _;
        socket_.cancel(_);
    }
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

} // namespace impl

template <typename T>
inline std::string_view error_message(T&& conn) {
    static_assert(Connection<T>, "T must be a Connection");
    if (is_null_recursive(conn)) {
        return {};
    }
    return impl::connection_error_message(get_native_handle(conn));
}

template <typename Connection, typename Executor>
inline error_code bind_executor(Connection& conn, const Executor& ex) {
    static_assert(ozo::Connection<Connection>, "conn should model Connection");
    return unwrap_connection(conn).bind_executor(ex);
}

template <typename Connection>
inline error_code close_connection(Connection&& conn) {
    static_assert(ozo::Connection<Connection>, "conn should model Connection");
    return unwrap_connection(conn).close();
}

template <typename T>
inline bool connection_bad(const T& conn) noexcept {
    static_assert(Connection<T>, "T must be a Connection");
    using impl::connection_status_bad;
    return is_null_recursive(conn) ? true : connection_status_bad(get_native_handle(conn));
}

template <typename Connection>
inline auto get_native_handle(const Connection& conn) noexcept {
    static_assert(ozo::Connection<Connection>, "conn should model Connection");
    return unwrap_connection(conn).native_handle();
}

template <typename Connection>
inline const auto& get_error_context(const Connection& conn) {
    static_assert(ozo::Connection<Connection>, "T must be a Connection");
    return unwrap_connection(conn).get_error_context();
}

template <typename Connection, typename Ctx>
inline void set_error_context(Connection& conn, Ctx&& ctx) {
    static_assert(ozo::Connection<Connection>, "conn should model Connection");
    unwrap_connection(conn).set_error_context(std::forward<Ctx>(ctx));
}

template <typename Connection>
inline void reset_error_context(Connection& conn) {
    static_assert(ozo::Connection<Connection>, "conn should model Connection");
    unwrap_connection(conn).set_error_context();
}

template <typename Connection>
inline decltype(auto) get_oid_map(Connection&& conn) noexcept {
    static_assert(ozo::Connection<Connection>, "conn should model Connection");
    return unwrap_connection(std::forward<Connection>(conn)).oid_map();
}

template <typename Connection>
inline decltype(auto) get_statistics(Connection&& conn) noexcept {
    static_assert(ozo::Connection<Connection>, "conn should model Connection");
    return unwrap_connection(std::forward<Connection>(conn)).statistics();
}

template <typename Connection>
inline auto get_executor(const Connection& conn) noexcept {
    static_assert(ozo::Connection<Connection>, "conn should model Connection");
    return unwrap_connection(conn).get_executor();
}

template <typename Connection>
inline std::string_view get_database(const Connection& conn) {
    static_assert(ozo::Connection<Connection>, "conn should model Connection");
    return PQdb(get_native_handle(conn));
}

template <typename Connection>
inline std::string_view get_host(const Connection& conn) {
    static_assert(ozo::Connection<Connection>, "conn should model Connection");
    return PQhost(get_native_handle(conn));
}

template <typename Connection>
inline std::string_view get_port(const Connection& conn) {
    static_assert(ozo::Connection<Connection>, "conn should model Connection");
    return PQport(get_native_handle(conn));
}

template <typename Connection>
inline std::string_view get_user(const Connection& conn) {
    static_assert(ozo::Connection<Connection>, "conn should model Connection");
    return PQuser(get_native_handle(conn));
}

template <typename Connection>
inline std::string_view get_password(const Connection& conn) {
    static_assert(ozo::Connection<Connection>, "conn should model Connection");
    return PQpass(get_native_handle(conn));
}

} // namespace ozo
