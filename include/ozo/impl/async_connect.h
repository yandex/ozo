#pragma once

#include <ozo/impl/io.h>
#include <ozo/impl/request_oid_map.h>
#include <ozo/connection.h>

namespace ozo {
namespace impl {

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
inline auto make_async_connect_op(Connection& conn,
        Handler&& handler) {
    return async_connect_op<std::decay_t<Handler>, std::decay_t<Connection>> {
        conn, std::forward<Handler>(handler)
    };
}

template <typename Handler, typename Connection>
inline void request_oid_map(Connection&& conn, Handler&& handler) {
    make_async_request_oid_map_op(std::forward<Handler>(handler))
        .perform(std::forward<Connection>(conn));
}

template <typename Handler, typename Connection>
struct connection_binder {
    Handler handler_;
    Connection conn_;

    void operator() (error_code ec) {
        using namespace hana::literals;
        if (ec || empty(get_oid_map(conn_))) {
            handler_(std::move(ec), std::move(conn_));
        } else {
            request_oid_map(std::move(conn_), std::move(handler_));
        }
    }

    template <typename Func>
    friend void asio_handler_invoke(Func&& f, connection_binder* ctx) {
        using ::boost::asio::asio_handler_invoke;
        asio_handler_invoke(std::forward<Func>(f),
            std::addressof(ctx->handler_));
    }
};

template <typename Handler, typename Connection>
inline auto bind_connection_handler(Handler&& base, Connection&& conn) {
    return connection_binder<std::decay_t<Handler>, std::decay_t<Connection>> {
        std::forward<Handler>(base), std::forward<Connection>(conn)
    };
}

template <typename T, typename Handler>
inline Require<Connectable<T>> async_connect(std::string conninfo, T&& conn,
        Handler&& handler) {

    decltype(auto) conn_ref = unwrap_connection(conn);
    make_async_connect_op(conn_ref,
        bind_connection_handler(std::forward<Handler>(handler), std::forward<T>(conn))
    ).perform(conninfo);
}

} // namespace impl
} // namespace ozo
