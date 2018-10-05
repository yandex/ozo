#pragma once

#include <ozo/native_conn_handle.h>
#include <ozo/connection.h>
#include <ozo/error.h>
#include <ozo/binary_query.h>
#include <ozo/detail/bind.h>
#include <ozo/impl/result_status.h>
#include <ozo/impl/result.h>

#include <boost/asio/post.hpp>

#include <libpq-fe.h>

namespace ozo::impl {

/**
* Query states. The values similar to PQflush function results.
* This state is used to synchronize async query parameters
* send process with async query result receiving process.
*/
enum class query_state : int {
    error = -1,
    send_finish = 0,
    send_in_progress = 1
};

namespace pq {

template <typename T>
inline decltype(auto) pq_connect_poll(T& conn) {
    static_assert(Connection<T>, "T must be a Connection");
    return PQconnectPoll(get_native_handle(conn));
}

template <typename T>
inline error_code pq_start_connection(T& conn, const std::string& conninfo) {
    static_assert(Connection<T>, "T must be a Connection");
    native_conn_handle handle(PQconnectStart(conninfo.c_str()));
    if (!handle) {
        return make_error_code(error::pq_connection_start_failed);
    }
    get_handle(conn) = std::move(handle);
    return {};
}

template <typename T>
inline error_code pq_assign_socket(T& conn) {
    static_assert(Connection<T>, "T must be a Connection");
    int fd = PQsocket(get_native_handle(conn));
    if (fd == -1) {
        return error::pq_socket_failed;
    }

    int new_fd = dup(fd);
    if (new_fd == -1) {
        set_error_context(conn, "error while dup(fd) for socket stream");
        return error_code{errno, boost::system::generic_category()};
    }

    error_code ec;
    get_socket(conn).assign(new_fd, ec);

    if (ec) {
        set_error_context(conn, "assign socket failed");
    }
    return ec;
}

template <typename T, typename ...Ts>
inline int pq_send_query_params(T& conn, const binary_query<Ts...>& q) noexcept {
    static_assert(Connection<T>, "T must be a Connection");
    return PQsendQueryParams(get_native_handle(conn),
                q.text(),
                q.params_count,
                q.types(),
                q.values(),
                q.lengths(),
                q.formats(),
                int(result_format::binary)
            );
}

template <typename T>
inline int pq_set_nonblocking(T& conn) noexcept {
    static_assert(Connection<T>, "T must be a Connection");
    return PQsetnonblocking(get_native_handle(conn), 1);
}

template <typename T>
inline int pq_consume_input(T& conn) noexcept {
    static_assert(Connection<T>, "T must be a Connection");
    return PQconsumeInput(get_native_handle(conn));
}

template <typename T>
inline bool pq_is_busy(T& conn) noexcept {
    static_assert(Connection<T>, "T must be a Connection");
    return PQisBusy(get_native_handle(conn));
}

template <typename T>
inline query_state pq_flush_output(T& conn) noexcept {
    static_assert(Connection<T>, "T must be a Connection");
    return static_cast<query_state>(PQflush(get_native_handle(conn)));
}

template <typename T>
inline native_result_handle pq_get_result(T& conn) noexcept {
    static_assert(Connection<T>, "T must be a Connection");
    return native_result_handle(PQgetResult(get_native_handle(conn)));
}

inline ExecStatusType pq_result_status(const PGresult& res) noexcept {
    return PQresultStatus(std::addressof(res));
}

inline error_code pq_result_error(const PGresult& res) noexcept {
    if (auto s = PQresultErrorField(std::addressof(res), PG_DIAG_SQLSTATE)) {
        return sqlstate::make_error_code(std::strtol(s, NULL, 36));
    }
    return error::no_sql_state_found;
}

} // namespace pq

template <typename T>
inline error_code start_connection(T& conn, const std::string& conninfo) {
    static_assert(Connection<T>, "T must be a Connection");
    using pq::pq_start_connection;
    return pq_start_connection(unwrap_connection(conn), conninfo);
}

template <typename T>
inline error_code assign_socket(T& conn) {
    static_assert(Connection<T>, "T must be a Connection");
    using pq::pq_assign_socket;
    return pq_assign_socket(unwrap_connection(conn));
}

template <typename T, typename Handler, typename = Require<Connection<T>>>
inline void write_poll(T& conn, Handler&& h) {
    get_socket(unwrap_connection(conn)).async_write_some(
            asio::null_buffers(), std::forward<Handler>(h));
}

template <typename T, typename Handler, typename = Require<Connection<T>>>
inline void read_poll(T& conn, Handler&& h) {
    get_socket(unwrap_connection(conn)).async_read_some(
            asio::null_buffers(), std::forward<Handler>(h));
}

// Shortcut for post operation with connection associated executor
template <typename T, typename Oper, typename = Require<Connection<T>>>
inline void post(T& conn, Oper&& op) {
    asio::post(get_io_context(conn), std::forward<Oper>(op));
}

template <typename T>
inline decltype(auto) connect_poll(T& conn) {
    static_assert(Connection<T>, "T must be a Connection");
    using pq::pq_connect_poll;
    return pq_connect_poll(unwrap_connection(conn));
}

template <typename T, typename Query>
inline decltype(auto) send_query_params(T& conn, Query&& q) noexcept {
    static_assert(Connection<T>, "T must be a Connection");
    using pq::pq_send_query_params;
    return pq_send_query_params(unwrap_connection(conn), std::forward<Query>(q));
}

template <typename T>
inline error_code set_nonblocking(T& conn) noexcept {
    static_assert(Connection<T>, "T must be a Connection");
    using pq::pq_set_nonblocking;
    if (pq_set_nonblocking(unwrap_connection(conn))) {
        return error::pg_set_nonblocking_failed;
    }
    return {};
}

template <typename T>
inline error_code consume_input(T& conn) noexcept {
    static_assert(Connection<T>, "T must be a Connection");
    using pq::pq_consume_input;
    if (!pq_consume_input(unwrap_connection(conn))) {
        return error::pg_consume_input_failed;
    }
    return {};
}

template <typename T>
inline bool is_busy(T& conn) noexcept {
    static_assert(Connection<T>, "T must be a Connection");
    using pq::pq_is_busy;
    return pq_is_busy(unwrap_connection(conn));
}

template <typename T>
inline query_state flush_output(T& conn) noexcept {
    static_assert(Connection<T>, "T must be a Connection");
    using pq::pq_flush_output;
    return pq_flush_output(unwrap_connection(conn));
}

template <typename T>
inline decltype(auto) get_result(T& conn) noexcept {
    static_assert(Connection<T>, "T must be a Connection");
    using pq::pq_get_result;
    return pq_get_result(unwrap_connection(conn));
}

template <typename T>
inline ExecStatusType result_status(T&& res) noexcept {
    using pq::pq_result_status;
    return pq_result_status(std::forward<T>(res));
}

template <typename T>
inline error_code result_error(T&& res) noexcept {
    using pq::pq_result_error;
    return pq_result_error(std::forward<T>(res));
}

} // namespace ozo::impl
