#pragma once

#include <ozo/detail/wrap_executor.h>
#include <ozo/detail/timeout_handler.h>
#include <ozo/detail/deadline.h>
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

    connect_operation_context(ConnectionT connection, Handler handler)
            : connection(std::move(connection)),
              handler(std::move(handler)) {}
};

template <typename ... Ts>
using connect_operation_context_ptr = std::shared_ptr<connect_operation_context<Ts ...>>;

template <typename Connection, typename Handler>
auto make_connect_operation_context(Connection&& connection, Handler&& handler) {
    using context_type = connect_operation_context<std::decay_t<Connection>, std::decay_t<Handler>>;
    auto allocator = asio::get_associated_allocator(handler);
    return std::allocate_shared<context_type>(allocator,
        std::forward<Connection>(connection),
        std::forward<Handler>(handler)
    );
}

template <typename ... Ts>
auto& get_connection(const connect_operation_context_ptr<Ts ...>& context) noexcept {
    return unwrap_connection(context->connection);
}

template <typename ... Ts>
auto& get_handler(const connect_operation_context_ptr<Ts ...>& context) noexcept {
    return context->handler;
}

/**
* Asynchronous connection operation
*/
template <typename Context>
struct async_connect_op {
    Context context;

    void perform(const std::string& conninfo) {
        auto handle = start_connection(get_connection(context), conninfo);
        if (!handle) {
            return done(error::pq_connection_start_failed);
        }

        using detail::connection_status_bad;
        if (connection_status_bad(handle.get())) {
            return done(error::pq_connection_status_bad);
        }

        if (error_code ec = get_connection(context).assign(std::move(handle)); ec) {
            return done(ec);
        }

        return get_connection(context).async_wait_write(std::move(*this));
    }

    void operator () (error_code ec, std::size_t = 0) {
        if (ec) {
            if (std::empty(get_error_context(get_connection(context)))) {
                get_connection(context).set_error_context("error while connection polling");
            }
            return done(ec);
        }

        switch (connect_poll(get_connection(context))) {
            case PGRES_POLLING_OK:
                return done();

            case PGRES_POLLING_WRITING:
                return get_connection(context).async_wait_write(std::move(*this));

            case PGRES_POLLING_READING:
                return get_connection(context).async_wait_read(std::move(*this));

            case PGRES_POLLING_FAILED:
            case PGRES_POLLING_ACTIVE:
                break;
        }

        done(error::pq_connect_poll_failed);
    }

    void done(error_code ec = error_code {}) {
        get_handler(context)(std::move(ec), std::move(context->connection));
    }

    using handler_type = std::decay_t<decltype(get_handler(context))>;

    using executor_type = asio::associated_executor_t<handler_type>;

    executor_type get_executor() const noexcept {
        return asio::get_associated_executor(get_handler(context));
    }

    using allocator_type = asio::associated_allocator_t<handler_type>;

    allocator_type get_allocator() const noexcept {
        return asio::get_associated_allocator(get_handler(context));
    }
};

template <typename Context>
inline auto make_async_connect_op(Context&& context) {
    return async_connect_op<std::decay_t<Context>> {std::forward<Context>(context)};
}

template <typename Connection, typename Handler>
inline void request_oid_map(Connection&& conn, Handler&& handler) {
    ozo::impl::request_oid_map_op op{std::forward<Handler>(handler)};
    op.perform(std::forward<Connection>(conn));
}

template <typename Handler>
struct request_oid_map_handler {
    Handler handler_;

    request_oid_map_handler(Handler handler) : handler_(std::move(handler)) {}

    template <typename Connection>
    void operator() (error_code ec, Connection&& conn) {
        if (ec) {
            handler_(std::move(ec), std::forward<Connection>(conn));
        } else {
            request_oid_map(std::forward<Connection>(conn), std::move(handler_));
        }
    }

    using executor_type = asio::associated_executor_t<Handler>;

    executor_type get_executor() const noexcept {
        return asio::get_associated_executor(handler_);
    }

    using allocator_type = asio::associated_allocator_t<Handler>;

    allocator_type get_allocator() const noexcept {
        return asio::get_associated_allocator(handler_);
    }
};

template <typename Connection>
constexpr bool OidMapEmpty = std::is_same_v<
    typename std::decay_t<decltype(ozo::unwrap_connection(std::declval<Connection>()))>::oid_map_type,
    ozo::empty_oid_map
>;

template <typename Conn, typename Handler>
constexpr auto apply_oid_map_request(Handler&& handler) {
    if constexpr (!OidMapEmpty<Conn>) {
        return request_oid_map_handler{std::forward<Handler>(handler)};
    } else {
        return std::forward<Handler>(handler);
    }
}

template <typename TimeConstraint, typename Connection, typename Handler>
inline auto apply_time_constaint(const TimeConstraint& t, [[maybe_unused]] Connection& conn, Handler&& handler) {
    if constexpr (IsNone<TimeConstraint>) {
        return detail::wrap_executor {get_executor(conn), std::forward<Handler>(handler)};
    } else {
        auto on_deadline = detail::cancel_io(unwrap_connection(conn), asio::get_associated_allocator(handler));
        return detail::deadline_handler {
            ozo::get_executor(conn), t, std::forward<Handler>(handler), std::move(on_deadline)
        };
    }
}

template <typename Connection, typename TimeConstraint, typename Handler>
inline void async_connect(std::string conninfo, const TimeConstraint& t,
        Connection&& conn, Handler&& handler) {
    static_assert(ozo::Connection<Connection>, "conn should model Connection concept");
    make_async_connect_op(
        make_connect_operation_context(
            std::forward<Connection>(conn),
            apply_oid_map_request<Connection>(
                apply_time_constaint(t, conn, std::forward<Handler>(handler))
            )
        )
    ).perform(conninfo);
}

} // namespace impl
} // namespace ozo
