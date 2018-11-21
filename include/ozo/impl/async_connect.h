#pragma once

#include <ozo/detail/cancel_timer_handler.h>
#include <ozo/detail/post_handler.h>
#include <ozo/detail/timeout_handler.h>
#include <ozo/impl/io.h>
#include <ozo/impl/request_oid_map.h>
#include <ozo/time_traits.h>
#include <ozo/connection.h>

#include <boost/asio/bind_executor.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/post.hpp>

namespace ozo {
namespace impl {

template <typename ConnectionT, typename Handler>
struct connect_operation_context {
    static_assert(Connection<ConnectionT>, "ConnectionT is not a Connection");

    ConnectionT connection;
    Handler handler;
    ozo::strand<decltype(get_io_context(connection))> strand {get_io_context(connection)};

    connect_operation_context(ConnectionT connection, Handler handler)
            : connection(std::move(connection)),
              handler(std::move(handler)) {}
};

template <typename ... Ts>
using connect_operation_context_ptr = std::shared_ptr<connect_operation_context<Ts ...>>;

template <typename Connection, typename Handler>
auto make_connect_operation_context(Connection&& connection, Handler&& handler) {
    using context_type = connect_operation_context<std::decay_t<Connection>, std::decay_t<Handler>>;
    return std::allocate_shared<context_type>(
        asio::get_associated_allocator(handler),
        std::forward<Connection>(connection),
        std::forward<Handler>(handler)
    );
}

template <typename ... Ts>
auto& get_connection(const connect_operation_context_ptr<Ts ...>& context) noexcept {
    return context->connection;
}

template <typename ... Ts>
auto& get_handler(const connect_operation_context_ptr<Ts ...>& context) noexcept {
    return context->handler;
}

template <typename ... Ts>
auto& get_executor(const connect_operation_context_ptr<Ts ...>& context) noexcept {
    return context->strand;
}

template <typename ... Ts>
auto get_allocator(const connect_operation_context_ptr<Ts ...>& context) noexcept {
    return asio::get_associated_allocator(get_handler(context));
}

/**
* Asynchronous connection operation
*/
template <typename Context>
struct async_connect_op {
    Context context;

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

        get_timer(get_connection(context)).expires_after(timeout);
        get_timer(get_connection(context)).async_wait(asio::bind_executor(get_executor(),
            detail::timeout_handler(get_socket(get_connection(context)))));

        return write_poll(get_connection(context), *this);
    }

    void operator () (error_code ec, std::size_t = 0) {
        if (ec) {
            if (std::empty(get_error_context(get_connection(context)))) {
                set_error_context(get_connection(context), "error while connection polling");
            }
            return done(ec);
        }

        switch (connect_poll(get_connection(context))) {
            case PGRES_POLLING_OK:
                return done();

            case PGRES_POLLING_WRITING:
                return write_poll(get_connection(context), *this);

            case PGRES_POLLING_READING:
                return read_poll(get_connection(context), *this);

            case PGRES_POLLING_FAILED:
            case PGRES_POLLING_ACTIVE:
                break;
        }

        done(error::pq_connect_poll_failed);
    }

    void done(error_code ec = error_code {}) {
        asio::post(get_executor(),
            detail::bind(
                std::move(get_handler(context)),
                std::move(ec), std::move(get_connection(context))
            )
        );
    }

    using executor_type = std::decay_t<decltype(impl::get_executor(context))>;

    executor_type get_executor() const noexcept {
        return impl::get_executor(context);
    }

    using allocator_type = std::decay_t<decltype(impl::get_allocator(context))>;

    allocator_type get_allocator() const noexcept {
        return impl::get_allocator(context);
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
        if (ec || empty(get_oid_map(conn))) {
            handler_(std::move(ec), std::forward<Connection>(conn));
        } else {
            request_oid_map(std::forward<Connection>(conn), std::move(handler_));
        }
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
            make_request_oid_map_handler(
                detail::cancel_timer_handler(
                    detail::post_handler(std::forward<Handler>(handler))
                )
            )
        )
    ).perform(conninfo, timeout);
}

} // namespace impl
} // namespace ozo
