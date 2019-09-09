#pragma once

#include <ozo/connection.h>
#include <ozo/impl/async_execute.h>
#include <ozo/impl/transaction.h>

namespace ozo::impl {

template <typename Handler, typename Options>
struct async_start_transaction_op {
    Handler handler;
    Options options;

    template <typename T, typename Query, typename TimeConstraint>
    void perform(T&& provider, Query&& query, TimeConstraint t) {
        static_assert(ConnectionProvider<T>, "T is not a ConnectionProvider");
        static_assert(ozo::TimeConstraint<TimeConstraint>, "should model TimeConstraint concept");
        async_execute(std::forward<T>(provider), std::forward<Query>(query),
            t, std::move(*this));
    }

    template <typename Connection>
    void operator ()(error_code ec, Connection&& connection) {
        asio::dispatch(
            detail::bind(
                std::move(handler),
                std::move(ec),
                make_transaction(std::forward<Connection>(connection), std::move(options))
            )
        );
    }
};

template <typename Handler, typename Options>
auto make_async_start_transaction_op(Handler&& handler, Options&& options) {
    return async_start_transaction_op<std::decay_t<Handler>, std::decay_t<Options>> {std::forward<Handler>(handler), std::forward<Options>(options)};
}

template <typename T, typename Options, typename Query, typename TimeConstraint, typename Handler>
Require<ConnectionProvider<T>> async_start_transaction(T&& provider, Options&& options, Query&& query,
        TimeConstraint t, Handler&& handler) {
    make_async_start_transaction_op(std::forward<Handler>(handler), std::forward<Options>(options))
        .perform(std::forward<T>(provider), std::forward<Query>(query), t);
}

struct initiate_async_start_transaction {
    template <typename Handler, typename ...Args>
    constexpr void operator()(Handler&& h, Args&& ...args) const {
        async_start_transaction(std::forward<Args>(args)..., std::forward<Handler>(h));
    }
};

} // namespace ozo::impl
