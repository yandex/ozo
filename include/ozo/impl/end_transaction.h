#pragma once

#include <ozo/impl/async_end_transaction.h>

namespace ozo::impl {

template <typename T, typename Query, typename TimeConstrain, typename CompletionToken>
auto end_transaction(transaction<T>&& transaction, Query&& query,
        TimeConstrain t, CompletionToken&& token) {
    using signature = void (error_code, T);

    async_completion<CompletionToken, signature> init(token);

    async_end_transaction(
        std::move(transaction),
        std::forward<Query>(query),
        t,
        init.completion_handler
    );

    return init.result.get();
}

} // namespace ozo::impl

