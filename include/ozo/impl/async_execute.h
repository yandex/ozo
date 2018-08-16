#pragma once

#include <ozo/impl/async_request.h>

namespace ozo {
namespace impl {

template <typename P, typename Q, typename Handler>
void async_execute(P&& provider, Q&& query, Handler&& handler) {
    static_assert(ConnectionProvider<P>, "is not a ConnectionProvider");
    static_assert(Query<Q> || QueryBuilder<Q>, "is neither Query nor QueryBuilder");
    async_get_connection(std::forward<P>(provider),
        impl::make_async_request_op(
            std::forward<Q>(query),
            [] (auto, auto) {},
            std::forward<Handler>(handler)
        )
    );
}

} // namespace impl
} // namespace ozo
