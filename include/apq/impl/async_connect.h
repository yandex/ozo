#pragma once

#include <apq/connection.h>
#include <apq/error.h>
#include <apq/detail/bind.h>
#include <libpq-fe.h>

namespace libapq {
namespace impl {
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

/**
* Asynchronous connection operation
*/
template <typename Handler, typename Connection>
struct async_connect_op {
    static_assert(Connectable<Connection>,
        "Connection type does not meet Connectable requirements");

    Connection& conn_;
    Handler handler_;

    void done(error_code ec = error_code{}) {
        get_io_context(conn_)
            .post(detail::bind(std::move(handler_), std::move(ec)));
    }

    void perform(const std::string& conninfo) {
        if (error_code ec = start_connection(conn_, conninfo)) {
            return done(ec);
        }

        if (connection_bad(conn_)) {
            return done(error::pq_connection_status_bad);
        }

        if (error_code ec = assign_socket(conn_)) {
            return done(ec);
        }

        return write_poll(conn_, std::move(*this));
    }

    void operator () (error_code ec, std::size_t = 0) {
        if (ec) {
            set_error_context(conn_, "error while connection polling");
            return done(ec);
        }

        switch (connect_poll(conn_)) {
        case PGRES_POLLING_OK:
            return done();

        case PGRES_POLLING_WRITING:
            return write_poll(conn_, std::move(*this));

        case PGRES_POLLING_READING:
            return read_poll(conn_, std::move(*this));

        case PGRES_POLLING_FAILED:
        case PGRES_POLLING_ACTIVE:
            break;
        }

        done(error::pq_connect_poll_failed);
    }

    template <typename Func>
    friend void asio_handler_invoke(Func&& f, async_connect_op* ctx) {
        using ::boost::asio::asio_handler_invoke;
        asio_handler_invoke(std::forward<Func>(f), std::addressof(ctx->handler_));
    }
};

template <typename Connection, typename Handler>
inline void async_connect(const std::string& conninfo, Connection& conn,
        Handler&& handler) {
    async_connect_op<std::decay_t<Handler>, std::decay_t<Connection>> {
        conn, std::forward<Handler>(handler)
    }.perform(conninfo);
}

} // namespace impl
} // namespace libapq
