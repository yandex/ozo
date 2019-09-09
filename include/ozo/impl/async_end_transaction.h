#pragma once

#include <ozo/connection.h>
#include <ozo/impl/async_execute.h>
#include <ozo/impl/transaction.h>

namespace ozo::impl {

template <typename Handler>
struct async_end_transaction_op {
    Handler handler;

    template <typename T, typename Query, typename TimeConstraint>
    void perform(T&& provider, Query&& query, TimeConstraint t) {
        static_assert(Connection<T>, "T is not a Connection");
        static_assert(ozo::TimeConstraint<TimeConstraint>, "should model TimeConstraint concept");
        async_execute(std::forward<T>(provider), std::forward<Query>(query), t, std::move(*this));
    }

    template <typename Connection, typename Options>
    void operator ()(error_code ec, impl::transaction<Connection, Options> transaction) {
        Connection connection;
        transaction.take_connection(connection);
        asio::dispatch(
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

template <typename T, typename Query, typename TimeConstraint, typename Handler>
Require<ConnectionProvider<T>> async_end_transaction(T&& provider, Query&& query,
        TimeConstraint t, Handler&& handler) {
    make_async_end_transaction_op(std::forward<Handler>(handler))
        .perform(std::forward<T>(provider), std::forward<Query>(query), t);
}

struct initiate_async_end_transaction {
    template <typename Handler, typename ...Args>
    constexpr void operator()(Handler&& h, Args&& ...args) const {
        async_end_transaction(std::forward<Args>(args)..., std::forward<Handler>(h));
    }
};

} // namespace ozo::impl
