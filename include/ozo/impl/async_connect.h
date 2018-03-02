#pragma once

#include <ozo/impl/io.h>

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
inline void async_connect(const std::string& conninfo, Connection& conn,
        Handler&& handler) {
    async_connect_op<std::decay_t<Handler>, std::decay_t<Connection>> {
        conn, std::forward<Handler>(handler)
    }.perform(conninfo);
}

} // namespace impl
} // namespace ozo
