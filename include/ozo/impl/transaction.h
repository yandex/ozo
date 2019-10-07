#pragma once

#include <ozo/transaction.h>
#include <ozo/connection.h>
#include <ozo/transaction_options.h>
#include <ozo/impl/async_execute.h>

namespace ozo {

template <typename... Ts>
struct is_nullable<transaction<Ts...>> : is_nullable<typename transaction<Ts...>::handle_type> {};

template <typename... Ts>
struct is_null_impl<transaction<Ts...>> {
    static bool apply(const transaction<Ts...>& v) noexcept {
        return v.is_null();
    }
};

namespace detail {

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
                ozo::transaction(std::forward<Connection>(connection), std::move(options))
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

template <typename Handler, typename ...Args>
constexpr void initiate_async_start_transaction::operator()(Handler&& h, Args&& ...args) const {
    async_start_transaction(std::forward<Args>(args)..., std::forward<Handler>(h));
}

template <typename Handler>
struct async_end_transaction_op {
    Handler handler;

    template <typename T, typename Query, typename TimeConstraint>
    void perform(T&& provider, Query&& query, TimeConstraint t) {
        static_assert(Connection<T>, "T is not a Connection");
        static_assert(ozo::TimeConstraint<TimeConstraint>, "should model TimeConstraint concept");
        using ozo::impl::async_execute;
        async_execute(std::forward<T>(provider), std::forward<Query>(query), t, std::move(*this));
    }

    template <typename Connection, typename Options>
    void operator ()(error_code ec, transaction<Connection, Options> transaction) {
        asio::dispatch(
            detail::bind(
                std::move(handler),
                std::move(ec),
                release_connection(std::move(transaction))
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

template <typename Handler, typename ...Args>
constexpr void initiate_async_end_transaction::operator()(Handler&& h, Args&& ...args) const {
    async_end_transaction(std::forward<Args>(args)..., std::forward<Handler>(h));
}

} // namespace detail
} // namespace ozo
