#pragma once

#include <ozo/native_result_handle.h>
#include <ozo/connection.h>
#include <ozo/error.h>
#include <ozo/detail/bind.h>
#include <libpq-fe.h>

namespace ozo {
namespace impl {

enum class result_format : int {
    text = 0,
    binary = 1,
};

namespace pq {

template <typename T, typename Handler, typename = Require<Connectable<T>>>
inline void pq_write_poll(T& conn, Handler&& h) {
    get_socket(conn).async_write_some(
            asio::null_buffers(), std::forward<Handler>(h));
}

template <typename T, typename Handler, typename = Require<Connectable<T>>>
inline void pq_read_poll(T& conn, Handler&& h) {
    get_socket(conn).async_read_some(
            asio::null_buffers(), std::forward<Handler>(h));
}

template <typename T, typename = Require<Connectable<T>>>
inline decltype(auto) pq_connect_poll(T& conn) {
    return PQconnectPoll(get_native_handle(conn));
}

template <typename T, typename = Require<Connectable<T>>>
inline error_code pq_start_connection(T& conn, const std::string& conninfo) {
    pg_conn_handle handle(PQconnectStart(conninfo.c_str()), PQfinish);
    if (!handle) {
        return make_error_code(error::pq_connection_start_failed);
    }
    get_handle(conn) = std::move(handle);
    return {};
}

template <typename T, typename = Require<Connectable<T>>>
inline error_code pq_assign_socket(T& conn) {
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

inline oid_t pq_field_type(const PGresult& res, int column) noexcept {
    return PQftype(std::addressof(res), column);
}

inline result_format pq_field_format(const PGresult& res, int column) noexcept {
    return static_cast<result_format>(PQfformat(std::addressof(res), column));
}

inline const char* pq_get_value(const PGresult& res, int row, int column) noexcept {
    return PQgetvalue(std::addressof(res), row, column);
}

inline std::size_t pq_get_length(const PGresult& res, int row, int column) noexcept {
    return PQgetlength(std::addressof(res), row, column);
}

inline bool pq_get_isnull(const PGresult& res, int row, int column) noexcept {
    return PQgetisnull(std::addressof(res), row, column);
}

inline int pq_field_number(const PGresult& res, const char* name) noexcept {
    return PQfnumber(std::addressof(res), name);
}

inline int pq_nfields(const PGresult& res) noexcept {
    return PQnfields(std::addressof(res));
}

inline int pq_ntuples(const PGresult& res) noexcept {
    return PQntuples(std::addressof(res));
}

} // namespace pq

template <typename T, typename = Require<Connectable<T>>>
inline error_code start_connection(T& conn, const std::string& conninfo) {
    using pq::pq_start_connection;
    return pq_start_connection(unwrap_connection(conn), conninfo);
}

template <typename T, typename = Require<Connectable<T>>>
inline error_code assign_socket(T& conn) {
    using pq::pq_assign_socket;
    return pq_assign_socket(unwrap_connection(conn));
}

template <typename T, typename Handler, typename = Require<Connectable<T>>>
inline void write_poll(T& conn, Handler&& h) {
    using pq::pq_write_poll;
    pq_write_poll(unwrap_connection(conn), std::forward<Handler>(h));
}

template <typename T, typename Handler, typename = Require<Connectable<T>>>
inline void read_poll(T& conn, Handler&& h) {
    using pq::pq_read_poll;
    pq_read_poll(unwrap_connection(conn), std::forward<Handler>(h));
}

template <typename T, typename = Require<Connectable<T>>>
inline decltype(auto) connect_poll(T& conn) {
    using pq::pq_connect_poll;
    return pq_connect_poll(unwrap_connection(conn));
}

template <typename T>
inline oid_t field_type(T&& res, int column) noexcept {
    using pq::pq_field_type;
    return pq_field_type(std::forward<T>(res), column);
}

template <typename T>
inline result_format field_format(T&& res, int column) noexcept {
    using pq::pq_field_format;
    return pq_field_format(std::forward<T>(res), column);
}

template <typename T>
inline const char* get_value(T&& res, int row, int column) noexcept {
    using pq::pq_get_value;
    return pq_get_value(std::forward<T>(res), row, column);
}

template <typename T>
inline std::size_t get_length(T&& res, int row, int column) noexcept {
    using pq::pq_get_length;
    return pq_get_length(std::forward<T>(res), row, column);
}

template <typename T>
inline bool get_isnull(T&& res, int row, int column) noexcept {
    using pq::pq_get_isnull;
    return pq_get_isnull(std::forward<T>(res), row, column);
}

template <typename T>
inline int field_number(T&& res, const char* name) noexcept {
    using pq::pq_field_number;
    return pq_field_number(std::forward<T>(res), name);
}

template <typename T>
inline int nfields(T&& res) noexcept {
    using pq::pq_nfields;
    return pq_nfields(std::forward<T>(res));
}

template <typename T>
inline int ntuples(T&& res) noexcept {
    using pq::pq_ntuples;
    return pq_ntuples(std::forward<T>(res));
}

} // namespace impl
} // namespace ozo
