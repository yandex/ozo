#pragma once

#include <ozo/connection.h>
#include <ozo/impl/async_execute.h>
#include <ozo/impl/transaction.h>

namespace ozo::impl {

template <typename Handler>
struct async_end_transaction_op {
    Handler handler;

    template <typename T, typename Query>
    void perform(T&& provider, Query&& query, const time_traits::duration& timeout) {
        static_assert(Connection<T>, "T is not a Connection");
        async_execute(std::forward<T>(provider), std::forward<Query>(query), timeout, std::move(*this));
    }

    template <typename Connection>
    void operator ()(error_code ec, impl::transaction<Connection> transaction) {
        Connection connection;
        transaction.take_connection(connection);
        auto ex = get_executor(connection);
        asio::post(ex,
            detail::bind(
                std::move(handler),
                std::move(ec),
                std::move(connection)
            )
        );
    }
};

template <typename Handler>
auto make_async_end_transaction_op(Handler&& handler) {
    return async_end_transaction_op<std::decay_t<Handler>> {std::forward<Handler>(handler)};
}

template <typename T, typename Query, typename Handler>
Require<ConnectionProvider<T>> async_end_transaction(T&& provider, Query&& query,
        const time_traits::duration& timeout, Handler&& handler) {
    make_async_end_transaction_op(std::forward<Handler>(handler))
        .perform(std::forward<T>(provider), std::forward<Query>(query), timeout);
}

} // namespace ozo::impl
