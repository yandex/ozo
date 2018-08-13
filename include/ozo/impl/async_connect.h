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
    static_assert(ozo::Connection<Connection>,
        "Connection type does not meet Connection requirements");

    Connection conn_;
    Handler handler_;

    void done(error_code ec = error_code{}) {
        get_io_context(conn_)
            .post(detail::bind(std::move(handler_), std::move(ec), std::move(conn_)));
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
        using boost::asio::asio_handler_invoke;
        asio_handler_invoke(std::forward<Func>(f), std::addressof(ctx->handler_));
    }
};

template <typename Connection, typename Handler>
inline auto make_async_connect_op(Connection&& conn,
        Handler&& handler) {
    return async_connect_op<std::decay_t<Handler>, std::decay_t<Connection>> {
        std::forward<Connection>(conn), std::forward<Handler>(handler)
    };
}

template <typename Handler, typename Connection>
inline void request_oid_map(Connection&& conn, Handler&& handler) {
    make_async_request_oid_map_op(std::forward<Handler>(handler))
        .perform(std::forward<Connection>(conn));
}

template <typename Handler>
struct request_oid_map_handler {
    Handler handler_;

    template <typename Connection>
    void operator() (error_code ec, Connection&& conn) {
        using namespace hana::literals;
        if (ec || empty(get_oid_map(conn))) {
            handler_(std::move(ec), std::forward<Connection>(conn));
        } else {
            request_oid_map(std::forward<Connection>(conn), std::move(handler_));
        }
    }

    template <typename Func>
    friend void asio_handler_invoke(Func&& f, request_oid_map_handler* ctx) {
        using boost::asio::asio_handler_invoke;
        asio_handler_invoke(std::forward<Func>(f),
            std::addressof(ctx->handler_));
    }
};

template <typename Handler>
inline auto make_request_oid_map_handler(Handler&& base) {
    return request_oid_map_handler<std::decay_t<Handler>> {std::forward<Handler>(base)};
}

template <typename T, typename Handler>
inline Require<Connection<T>> async_connect(std::string conninfo, T&& conn,
        Handler&& handler) {
    make_async_connect_op(
        std::forward<T>(conn),
        make_request_oid_map_handler(std::forward<Handler>(handler))
    ).perform(conninfo);
}

} // namespace impl
} // namespace ozo
