#pragma once

#include <ozo/connection.h>
#include <ozo/impl/async_request.h>

namespace ozo {

template <typename P, typename Q, typename Out, typename Handler>
inline void async_request(P&& provider, Q&& query, Out&& out,
        Handler&& handler) {
    static_assert(ConnectionProvider<P>, "is not a ConnectionProvider");
    static_assert(Query<Q> || QueryBuilder<Q>, "is neither Query nor QueryBuilder");
    async_get_connection(std::forward<P>(provider),
        impl::make_async_request_op(
            std::forward<Q>(query),
            std::forward<Out>(out),
            std::forward<Handler>(handler)
        )
    );
}

} // namespace ozo
