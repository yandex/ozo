#pragma once

#include <apq/connection.h>
#include <apq/impl/async_connect.h>

namespace libapq {
namespace detail {

template <typename Handler, typename Connection>
struct connection_binder {
    Handler handler_;
    Connection conn_;

    void operator() (error_code ec) {
        handler_(std::move(ec), std::move(conn_));
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

} // namespace detail

template <typename T, typename Handler>
inline Require<Connectable<T>> async_connect(std::string conn_info, T&& conn,
        Handler&& handler) {

    decltype(auto) conn_ref = unwrap_connection(conn);
    impl::async_connect(conn_info, conn_ref,
            detail::bind_connection_handler(std::forward<Handler>(handler),
                std::forward<T>(conn)));
}

} // namespace libapq
