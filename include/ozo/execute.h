#pragma once

#include <ozo/impl/async_execute.h>

namespace ozo {

template <typename P, typename Q, typename CompletionToken, typename = Require<ConnectionProvider<P>>>
auto execute(P&& provider, Q&& query, CompletionToken&& token) {
    using signature_t = void (error_code, connection_type<P>);
    async_completion<CompletionToken, signature_t> init(token);

    impl::async_execute(std::forward<P>(provider), std::forward<Q>(query), init.completion_handler);

    return init.result.get();
}

} // namespace ozo
