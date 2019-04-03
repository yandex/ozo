#pragma once

#include <ozo/impl/async_execute.h>

namespace ozo {

#ifdef OZO_DOCUMENTATION
/**
 * @brief Executes query but does not return a result
 * @ingroup group-requests-functions
 *
 * This function is same as `ozo::request()` function except it does not return any result.
 * It suitable to use with `UPDATE` `INSERT` statements, or invoking procedures without result.
 *
 * @param provider --- #ConnectionProvider object
 * @param query --- #Query to execute
 * @param timeout --- request timeout
 * @param token --- operation #CompletionToken.
 * @return depends on #CompletionToken.
 */
template <typename P, typename Q, typename CompletionToken>
decltype(auto) execute(P&& provider, Q&& query, const time_traits::duration& timeout, CompletionToken&& token);

/**
 * @brief Executes query but does not return a result
 * @ingroup group-requests-functions
 *
 * This function is same as `ozo::request()` function except it does not return any result.
 * It suitable to use with `UPDATE` `INSERT` statements, or invoking procedures without result.
 *
 * @param provider --- #ConnectionProvider object
 * @param query --- #Query to execute
 * @param token --- operation #CompletionToken.
 * @return depends on #CompletionToken.
 */
template <typename P, typename Q, typename CompletionToken>
decltype(auto) execute(P&& provider, Q&& query, CompletionToken&& token);
#else
struct execute_op {
    template <typename P, typename Q, typename CompletionToken>
    decltype(auto) operator() (P&& provider, Q&& query, const time_traits::duration& timeout, CompletionToken&& token) const {
        static_assert(ConnectionProvider<P>, "provider should be a ConnectionProvider");
        using signature_t = void (error_code, connection_type<P>);
        async_completion<CompletionToken, signature_t> init(token);

        impl::async_execute(std::forward<P>(provider), std::forward<Q>(query), timeout, init.completion_handler);

        return init.result.get();
    }

    template <typename P, typename Q, typename CompletionToken>
    decltype(auto) operator() (P&& provider, Q&& query, CompletionToken&& token) const {
        static_assert(ConnectionProvider<P>, "provider should be a ConnectionProvider");
        return (*this)(
            std::forward<P>(provider),
            std::forward<Q>(query),
            time_traits::duration::max(),
            std::forward<CompletionToken>(token)
        );
    }
};

constexpr execute_op execute;
#endif
} // namespace ozo
