#pragma once

#include <ozo/connection.h>
#include <ozo/impl/async_execute.h>
#include <ozo/impl/transaction.h>

namespace ozo::impl {

template <typename Handler>
struct async_start_transaction_op {
    Handler handler;

    template <typename T, typename Query, typename TimeConstrain>
    void perform(T&& provider, Query&& query, TimeConstrain&& time_constrain) {
        static_assert(ConnectionProvider<T>, "T is not a ConnectionProvider");
        async_execute(std::forward<T>(provider), std::forward<Query>(query),
            std::forward<TimeConstrain>(time_constrain), std::move(*this));
    }

    template <typename Connection>
    void operator ()(error_code ec, Connection&& connection) {
        asio::dispatch(
            detail::bind(
                std::move(handler),
                std::move(ec),
                make_transaction(std::forward<Connection>(connection))
            )
        );
    }
};

template <typename Handler>
auto make_async_start_transaction_op(Handler&& handler) {
    return async_start_transaction_op<std::decay_t<Handler>> {std::forward<Handler>(handler)};
}

template <typename T, typename Query, typename TimeConstrain, typename Handler>
Require<ConnectionProvider<T>> async_start_transaction(T&& provider, Query&& query,
        TimeConstrain&& time_constrain, Handler&& handler) {
    make_async_start_transaction_op(std::forward<Handler>(handler))
        .perform(std::forward<T>(provider), std::forward<Query>(query),
            std::forward<TimeConstrain>(time_constrain));
}

} // namespace ozo::impl
