#pragma once

#include <ozo/impl/async_execute.h>

namespace ozo {

/**
 * @brief Executes query but does not return a result
 * @ingroup group-requests
 *
 * This function is same as `ozo::request()` function except it does not return any result.
 * It suitable to use with `UPDATE` `INSERT` statements, or invoking procedures without result.
 *
 * @param provider --- #ConnectionProvider object
 * @param query --- #Query to execute
 * @param token --- any valid of #CompletionToken.
 * @return depends on #CompletionToken.
 */
template <typename P, typename Q, typename CompletionToken, typename = Require<ConnectionProvider<P>>>
inline auto execute(P&& provider, Q&& query, const time_traits::duration& timeout, CompletionToken&& token) {
    using signature_t = void (error_code, connection_type<P>);
    async_completion<CompletionToken, signature_t> init(token);

    impl::async_execute(std::forward<P>(provider), std::forward<Q>(query), timeout, init.completion_handler);

    return init.result.get();
}

template <typename P, typename Q, typename CompletionToken, typename = Require<ConnectionProvider<P>>>
inline auto execute(P&& provider, Q&& query, CompletionToken&& token) {
    return execute(
        std::forward<P>(provider),
        std::forward<Q>(query),
        time_traits::duration::max(),
        std::forward<CompletionToken>(token)
    );
}

} // namespace ozo
