#pragma once

#include <ozo/detail/do_nothing.h>
#include <ozo/impl/async_request.h>
#include <ozo/deadline.h>

namespace ozo {
namespace impl {

template <typename P, typename Q, typename Handler>
inline void async_execute(P&& provider, Q&& query, const time_traits::duration& timeout, Handler&& handler) {
    static_assert(ConnectionProvider<P>, "is not a ConnectionProvider");
    static_assert(Query<Q> || QueryBuilder<Q>, "is neither Query nor QueryBuilder");
    async_get_connection(std::forward<P>(provider),
        async_request_op {
            std::forward<Q>(query),
            timeout,
            detail::do_nothing {},
            std::forward<Handler>(handler)
        }
    );
}

template <typename P, typename Q, typename Handler>
inline void async_execute(P&& provider, Q&& query, const deadline& at, Handler&& handler) {
    static_assert(ConnectionProvider<P>, "is not a ConnectionProvider");
    static_assert(Query<Q> || QueryBuilder<Q>, "is neither Query nor QueryBuilder");
    async_get_connection(std::forward<P>(provider), at,
        async_request_op {
            std::forward<Q>(query),
            at,
            detail::do_nothing {},
            std::forward<Handler>(handler)
        }
    );
}

} // namespace impl
} // namespace ozo
