#pragma once

#include <ozo/impl/async_execute.h>

namespace ozo {

#ifdef OZO_DOCUMENTATION
/**
 * @brief Executes query with no result data expected
 *
 * This function is same as `ozo::request()` function except it does not provide any result data.
 * It suitable to use with `UPDATE` `INSERT` statements, or invoking procedures without result.
 *
 * @note The function does not particitate in ADL since could be implemented via functional object.
 *
 * @param provider --- #ConnectionProvider object
 * @param query --- #Query to execute
 * @param time_constraint --- request #TimeConstraint; this time constrain <b>includes</b> time for getting connection from provider.
 * @param token --- operation #CompletionToken.
 * @return deduced from #CompletionToken.
 * @ingroup group-requests-functions
 */
template <typename ConnectionProvider, typename Query, typename TimeConstraint, typename CompletionToken>
decltype(auto) execute(ConnectionProvider&& provider, Query&& query, TimeConstraint time_constraint, CompletionToken&& token);

/**
 * @brief Executes query with no result data expected
 *
 * This function is time constrain free shortcut to `ozo::execute()` function.
 * Its call is equal to `ozo::execute(provider, query, ozo::none, token)` call.
 *
 * @note The function does not particitate in ADL since could be implemented via functional object.
 *
 * @param provider --- #ConnectionProvider object
 * @param query --- #Query to execute
 * @param token --- operation #CompletionToken.
 * @return deduced from #CompletionToken.
 * @ingroup group-requests-functions
 */
template <typename P, typename Q, typename CompletionToken>
decltype(auto) execute(P&& provider, Q&& query, CompletionToken&& token);
#else
struct execute_op {
    template <typename P, typename Q, typename TimeConstraint, typename CompletionToken>
    decltype(auto) operator() (P&& provider, Q&& query, TimeConstraint t, CompletionToken&& token) const {
        static_assert(ConnectionProvider<P>, "provider should be a ConnectionProvider");
        static_assert(ozo::TimeConstraint<TimeConstraint>, "should model TimeConstraint concept");
        using signature_t = void (error_code, connection_type<P>);
        async_completion<CompletionToken, signature_t> init(token);

        impl::async_execute(std::forward<P>(provider), std::forward<Q>(query),
            t, init.completion_handler);

        return init.result.get();
    }

    template <typename P, typename Q, typename CompletionToken>
    decltype(auto) operator() (P&& provider, Q&& query, CompletionToken&& token) const {
        return (*this)(std::forward<P>(provider), std::forward<Q>(query), none,
            std::forward<CompletionToken>(token));
    }
};

constexpr execute_op execute;
#endif
} // namespace ozo

