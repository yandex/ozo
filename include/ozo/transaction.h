#pragma once

#include <ozo/impl/start_transaction.h>
#include <ozo/impl/end_transaction.h>

namespace ozo {

template <typename T, typename CompletionToken, typename = Require<ConnectionProvider<T>>>
auto begin(T&& provider, const time_traits::duration& timeout, CompletionToken&& token) {
    using namespace ozo::literals;
    return impl::start_transaction(
        std::forward<T>(provider),
        "BEGIN"_SQL,
        timeout,
        std::forward<CompletionToken>(token)
    );
}

template <typename T, typename CompletionToken, typename = Require<ConnectionProvider<T>>>
auto begin(T&& provider, CompletionToken&& token) {
    return begin(
        std::forward<T>(provider),
        time_traits::duration::max(),
        std::forward<CompletionToken>(token)
    );
}

template <typename T, typename CompletionToken>
auto commit(impl::transaction<T>&& transaction, const time_traits::duration& timeout, CompletionToken&& token) {
    using namespace ozo::literals;
    return impl::end_transaction(
        std::move(transaction),
        "COMMIT"_SQL,
        timeout,
        std::forward<CompletionToken>(token)
    );
}

template <typename T, typename CompletionToken>
auto commit(impl::transaction<T>&& transaction, CompletionToken&& token) {
    return commit(
        std::move(transaction),
        time_traits::duration::max(),
        std::forward<CompletionToken>(token)
    );
}

template <typename T, typename CompletionToken>
auto rollback(impl::transaction<T>&& transaction, const time_traits::duration& timeout, CompletionToken&& token) {
    using namespace ozo::literals;
    return impl::end_transaction(
        std::move(transaction),
        "ROLLBACK"_SQL,
        timeout,
        std::forward<CompletionToken>(token)
    );
}

template <typename T, typename CompletionToken>
auto rollback(impl::transaction<T>&& transaction, CompletionToken&& token) {
    return rollback(
        std::move(transaction),
        time_traits::duration::max(),
        std::forward<CompletionToken>(token)
    );
}

} // namespace ozo
