#pragma once

#include <ozo/connection.h>
#include <ozo/request.h>

namespace ozo {
namespace impl {

template <typename T>
struct transaction {
    static_assert(Connection<T>, "is not a Connection");

    T conn_;

    friend auto& unwrap_connection(transaction& self) noexcept {
        return unwrap_connection(self.conn_);
    }

    friend const auto& unwrap_connection(const transaction& self) noexcept {
        return unwrap_connection(self.conn_);
    }
};

} // namespace impl

namespace impl {

template <typename Conn, typename = Require<Connection<Conn>>>
auto make_transaction(Conn&& conn) {
    return transaction<Conn> {std::forward<Conn>(conn)};
}

} // namespace impl

template <typename P, typename CompletionToken, typename = Require<ConnectionProvider<P>>>
auto transaction(P&& provider, CompletionToken&& token) {
    using namespace ozo::literals;
    using signature_t = void (error_code, impl::transaction<connection_type<P>>);

    const auto result = std::make_shared<ozo::result>();
    async_completion<CompletionToken, signature_t> init(token);

    request(std::forward<P>(provider), "BEGIN"_SQL, *result,
        [handler = init.completion_handler, result] (error_code ec, auto&& conn) mutable {
            handler(std::move(ec), impl::make_transaction(std::move(conn)));
        });

    return init.result.get();
}

namespace detail {

template <typename Conn, typename Query, typename CompletionToken, typename = Require<Connection<Conn>>>
auto end_transaction(impl::transaction<Conn>&& transaction, Query&& query, CompletionToken&& token) {
    using signature_t = void (error_code, std::decay_t<Conn>);

    const auto result = std::make_shared<ozo::result>();
    async_completion<CompletionToken, signature_t> init(token);

    request(std::move(transaction), std::forward<Query>(query), *result,
        [handler = init.completion_handler, result] (error_code ec, auto transaction) mutable {
            handler(std::move(ec), std::move(transaction.conn_));
        });

    return init.result.get();
}

} // namespace detail

template <typename Conn, typename CompletionToken, typename = Require<Connection<Conn>>>
auto commit(impl::transaction<Conn>&& transaction, CompletionToken&& token) {
    using namespace ozo::literals;
    return detail::end_transaction(
        std::move(transaction),
        "COMMIT"_SQL,
        std::forward<CompletionToken>(token)
    );
}

template <typename Conn, typename CompletionToken, typename = Require<Connection<Conn>>>
auto rollback(impl::transaction<Conn>&& transaction, CompletionToken&& token) {
    using namespace ozo::literals;
    return detail::end_transaction(
        std::move(transaction),
        "ROLLBACK"_SQL,
        std::forward<CompletionToken>(token)
    );
}

} // namespace ozo
