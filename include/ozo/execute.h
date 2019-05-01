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
namespace detail {

struct initiate_async_execute {
    template <typename Handler, typename ...Args>
    constexpr void operator()(Handler&& h, Args&& ...args) const {
        impl::async_execute(std::forward<Args>(args)..., std::forward<Handler>(h));
    }
};

} // namespace detail

struct execute_op {
    template <typename P, typename Q, typename CompletionToken>
    decltype(auto) operator() (P&& provider, Q&& query,
            const time_traits::duration& timeout, CompletionToken&& token) const {
        static_assert(ConnectionProvider<P>, "provider should be a ConnectionProvider");
        return async_initiate<std::decay_t<CompletionToken>, handler_signature<P>>(
            detail::initiate_async_execute{}, token,
            std::forward<P>(provider), std::forward<Q>(query), timeout);
    }

    template <typename P, typename Q, typename CompletionToken>
    decltype(auto) operator() (P&& provider, Q&& query, deadline t, CompletionToken&& token) const {
        static_assert(ConnectionProvider<P>, "provider should be a ConnectionProvider");
        return async_initiate<std::decay_t<CompletionToken>, handler_signature<P>>(
            detail::initiate_async_execute{}, token,
            std::forward<P>(provider), std::forward<Q>(query), t);
    }

    template <typename P, typename Q, typename CompletionToken>
    decltype(auto) operator() (P&& provider, Q&& query, CompletionToken&& token) const {
        static_assert(ConnectionProvider<P>, "provider should be a ConnectionProvider");
        return async_initiate<std::decay_t<CompletionToken>, handler_signature<P>>(
            detail::initiate_async_execute{}, token,
            std::forward<P>(provider), std::forward<Q>(query), no_time_constrain);
    }
};

constexpr execute_op execute;
#endif
} // namespace ozo
