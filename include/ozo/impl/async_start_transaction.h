#pragma once

#include <ozo/connection.h>
#include <ozo/impl/async_execute.h>
#include <ozo/impl/transaction.h>

namespace ozo::impl {

template <typename Handler>
struct async_start_transaction_op {
    Handler handler;

    template <typename T, typename Query>
    void perform(T&& provider, Query&& query) {
        static_assert(ConnectionProvider<T>, "T is not a ConnectionProvider");
        async_execute(std::forward<T>(provider), std::forward<Query>(query), std::move(*this));
    }

    template <typename Connection>
    void operator ()(error_code ec, Connection&& connection) {
        get_io_context(connection).post(
            detail::bind(
                std::move(handler),
                std::move(ec),
                make_transaction(std::forward<Connection>(connection))
            )
        );
    }

    template <typename Function>
    friend void asio_handler_invoke(Function&& function, async_start_transaction_op* operation) {
        using boost::asio::asio_handler_invoke;
        asio_handler_invoke(std::forward<Function>(function), std::addressof(operation->handler));
    }
};

template <typename Handler>
auto make_async_start_transaction_op(Handler&& handler) {
    using result_type = async_start_transaction_op<std::decay_t<Handler>>;
    return result_type {std::forward<Handler>(handler)};
}

template <typename T, typename Query, typename Handler>
Require<ConnectionProvider<T>> async_start_transaction(T&& provider, Query&& query, Handler&& handler) {
    make_async_start_transaction_op(std::forward<Handler>(handler))
        .perform(std::forward<T>(provider), std::forward<Query>(query));
}

} // namespace ozo::impl
