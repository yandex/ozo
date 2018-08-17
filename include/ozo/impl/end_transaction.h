#pragma once

#include <ozo/impl/async_end_transaction.h>

namespace ozo::impl {

template <typename T, typename Query, typename CompletionToken>
auto end_transaction(transaction<T>&& transaction, Query&& query,
        const time_traits::duration& timeout, CompletionToken&& token) {
    using signature = void (error_code, T);

    async_completion<CompletionToken, signature> init(token);

    async_end_transaction(
        std::move(transaction),
        std::forward<Query>(query),
        timeout,
        init.completion_handler
    );

    return init.result.get();
}

} // namespace ozo::impl
