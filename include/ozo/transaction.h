#pragma once

#include <ozo/impl/async_start_transaction.h>
#include <ozo/impl/async_end_transaction.h>

namespace ozo {

template <typename T, typename Query, typename CompletionToken, typename = Require<ConnectionProvider<T>>>
auto start_transaction(T&& provider, Query&& query, CompletionToken&& token) {
    using signature = void (error_code, impl::transaction<connection_type<T>>);

    async_completion<CompletionToken, signature> init(token);

    impl::async_start_transaction(
        std::forward<T>(provider),
        std::forward<Query>(query),
        init.completion_handler
    );

    return init.result.get();
}

template <typename T, typename CompletionToken, typename = Require<ConnectionProvider<T>>>
auto begin(T&& provider, CompletionToken&& token) {
    using namespace ozo::literals;
    return start_transaction(
        std::forward<T>(provider),
        "BEGIN"_SQL,
        std::forward<CompletionToken>(token)
    );
}

template <typename T, typename Query, typename CompletionToken>
auto end_transaction(impl::transaction<T>&& transaction, Query&& query, CompletionToken&& token) {
    using signature = void (error_code, T);

    async_completion<CompletionToken, signature> init(token);

    impl::async_end_transaction(
        std::move(transaction),
        std::forward<Query>(query),
        init.completion_handler
    );

    return init.result.get();
}

template <typename T, typename CompletionToken>
auto commit(impl::transaction<T>&& transaction, CompletionToken&& token) {
    using namespace ozo::literals;
    return end_transaction(
        std::move(transaction),
        "COMMIT"_SQL,
        std::forward<CompletionToken>(token)
    );
}

template <typename T, typename CompletionToken>
auto rollback(impl::transaction<T>&& transaction, CompletionToken&& token) {
    using namespace ozo::literals;
    return end_transaction(
        std::move(transaction),
        "ROLLBACK"_SQL,
        std::forward<CompletionToken>(token)
    );
}

} // namespace ozo
