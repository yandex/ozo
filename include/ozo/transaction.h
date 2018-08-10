#pragma once

#include <ozo/impl/start_transaction.h>
#include <ozo/impl/end_transaction.h>

namespace ozo {

template <typename T, typename CompletionToken, typename = Require<ConnectionProvider<T>>>
auto begin(T&& provider, CompletionToken&& token) {
    using namespace ozo::literals;
    return impl::start_transaction(
        std::forward<T>(provider),
        "BEGIN"_SQL,
        std::forward<CompletionToken>(token)
    );
}

template <typename T, typename CompletionToken>
auto commit(impl::transaction<T>&& transaction, CompletionToken&& token) {
    using namespace ozo::literals;
    return impl::end_transaction(
        std::move(transaction),
        "COMMIT"_SQL,
        std::forward<CompletionToken>(token)
    );
}

template <typename T, typename CompletionToken>
auto rollback(impl::transaction<T>&& transaction, CompletionToken&& token) {
    using namespace ozo::literals;
    return impl::end_transaction(
        std::move(transaction),
        "ROLLBACK"_SQL,
        std::forward<CompletionToken>(token)
    );
}

} // namespace ozo
