#pragma once

#include <ozo/impl/async_request.h>

namespace ozo {

template <typename P, typename Q, typename Out, typename CompletionToken, typename = Require<ConnectionProvider<P>>>
inline auto request(P&& provider, Q&& query, Out&& out, CompletionToken&& token) {
    using signature_t = void (error_code, connectable_type<P>);
    async_completion<CompletionToken, signature_t> init(token);

    impl::async_request(std::forward<P>(provider), std::forward<Q>(query), std::forward<Out>(out),
                  init.completion_handler);

    return init.result.get();
}

} // namespace ozo
