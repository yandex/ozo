#pragma once

#include <ozo/impl/async_request.h>

namespace ozo {

template <typename P, typename Q, typename Out, typename CompletionToken, typename = Require<ConnectionProvider<P>>>
inline auto request(P&& provider, Q&& query, const time_traits::duration& timeout, Out out, CompletionToken&& token) {
    using signature_t = void (error_code, connection_type<P>);
    async_completion<CompletionToken, signature_t> init(token);

    impl::async_request(std::forward<P>(provider), std::forward<Q>(query), timeout, std::move(out),
            init.completion_handler);

    return init.result.get();
}

template <typename P, typename Q, typename Out, typename CompletionToken, typename = Require<ConnectionProvider<P>>>
inline auto request(P&& provider, Q&& query, Out out, CompletionToken&& token) {
    return request(
        std::forward<P>(provider),
        std::forward<Q>(query),
        time_traits::duration::max(),
        std::move(out),
        std::forward<CompletionToken>(token)
    );
}

} // namespace ozo
