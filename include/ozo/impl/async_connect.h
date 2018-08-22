#pragma once

#include <ozo/impl/io.h>
#include <ozo/impl/request_oid_map.h>
#include <ozo/time_traits.h>
#include <ozo/connection.h>

#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>

namespace ozo {
namespace impl {

template <typename ConnectionT, typename Handler, typename Timer>
struct connect_operation_context {
    static_assert(Connection<ConnectionT>, "ConnectionT is not a Connection");

    ConnectionT connection;
    Handler handler;
    Timer timer;
    ozo::strand<decltype(get_io_context(connection))> strand {get_io_context(connection)};

    connect_operation_context(ConnectionT connection, Handler handler, Timer timer)
            : connection(std::move(connection)),
              handler(std::move(handler)),
              timer(std::move(timer)) {}
};

template <typename ... Ts>
using connect_operation_context_ptr = std::shared_ptr<connect_operation_context<Ts ...>>;

template <typename Connection, typename Handler, typename Timer>
auto make_connect_operation_context(Connection&& connection, Handler&& handler, Timer&& timer) {
    using context_type = connect_operation_context<std::decay_t<Connection>, std::decay_t<Handler>, std::decay_t<Timer>>;
    return std::make_shared<context_type>(
        std::forward<Connection>(connection),
        std::forward<Handler>(handler),
        std::forward<Timer>(timer)
    );
}

template <typename ... Ts>
auto& get_connection(const connect_operation_context_ptr<Ts ...>& context) {
    return context->connection;
}

template <typename ... Ts>
auto& get_handler(const connect_operation_context_ptr<Ts ...>& context) {
    return context->handler;
}

template <typename ... Ts>
decltype(auto) get_handler_context(const connect_operation_context_ptr<Ts ...>& context) {
    return std::addressof(context->handler);
}

template <typename ... Ts>
auto& get_executor(const connect_operation_context_ptr<Ts ...>& context) {
    return context->strand;
}

template <typename ... Ts>
auto& get_timer(const connect_operation_context_ptr<Ts ...>& context) {
    return context->timer;
}

template <typename Socket>
struct timeout_handler {
    Socket& socket;

    void operator() (error_code ec) {
        if (ec == asio::error::operation_aborted) {
            return;
        } else {
            socket.cancel(ec);
        }
    }
};

template <typename Socket>
inline auto make_timeout_handler(Socket& socket) {
    return timeout_handler<std::decay_t<Socket>> {socket};
}

/**
* Asynchronous connection operation
*/
template <typename Context>
struct async_connect_op {
    Context context;

    void done(error_code ec = error_code {}) {
        get_timer(context).cancel();

        get_io_context(get_connection(context))
            .post(detail::bind(std::move(get_handler(context)), std::move(ec), std::move(get_connection(context))));
    }

    void perform(const std::string& conninfo, const time_traits::duration& timeout) {
        if (error_code ec = start_connection(get_connection(context), conninfo)) {
            return done(ec);
        }

        if (connection_bad(get_connection(context))) {
            return done(error::pq_connection_status_bad);
        }

        if (error_code ec = assign_socket(get_connection(context))) {
            return done(ec);
        }

        get_timer(context).expires_after(timeout);
        get_timer(context).async_wait(bind_executor(get_executor(context),
            make_timeout_handler(get_socket(get_connection(context)))));

        return write_poll(get_connection(context), bind_executor(get_executor(context), *this));
    }

    void operator () (error_code ec, std::size_t = 0) {
        if (ec) {
            set_error_context(get_connection(context), "error while connection polling");
            return done(ec);
        }

        switch (connect_poll(get_connection(context))) {
            case PGRES_POLLING_OK:
                return done();

            case PGRES_POLLING_WRITING:
                return write_poll(get_connection(context), std::move(*this));

            case PGRES_POLLING_READING:
                return read_poll(get_connection(context), std::move(*this));

            case PGRES_POLLING_FAILED:
            case PGRES_POLLING_ACTIVE:
                break;
        }

        done(error::pq_connect_poll_failed);
    }

    template <typename Function>
    friend void asio_handler_invoke(Function&& function, async_connect_op* operation) {
        using boost::asio::asio_handler_invoke;
        asio_handler_invoke(std::forward<Function>(function), get_handler_context(operation->context));
    }
};

template <typename Context>
inline auto make_async_connect_op(Context&& context) {
    return async_connect_op<std::decay_t<Context>> {std::forward<Context>(context)};
}

template <typename Connection, typename Handler>
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
        asio_handler_invoke(std::forward<Func>(f), std::addressof(ctx->handler_));
    }
};

template <typename Handler>
inline auto make_request_oid_map_handler(Handler&& handler) {
    return request_oid_map_handler<std::decay_t<Handler>> {std::forward<Handler>(handler)};
}

template <typename ConnectionT, typename Handler>
inline Require<Connection<ConnectionT>> async_connect(std::string conninfo, const time_traits::duration& timeout,
        ConnectionT&& connection, Handler&& handler) {
    make_async_connect_op(
        make_connect_operation_context(
            std::forward<ConnectionT>(connection),
            make_request_oid_map_handler(std::forward<Handler>(handler)),
            asio::steady_timer(get_io_context(connection))
        )
    ).perform(conninfo, timeout);
}

} // namespace impl
} // namespace ozo
