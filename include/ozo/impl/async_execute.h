#pragma once

#include <ozo/core/none.h>
#include <ozo/impl/async_request.h>

namespace ozo::impl {

template <typename P, typename Q, typename TimeConstraint, typename Handler>
inline void async_execute(P&& provider, Q&& query, TimeConstraint t, Handler&& handler) {
    static_assert(ConnectionProvider<P>, "is not a ConnectionProvider");
    static_assert(BinaryQueryConvertible<Q>, "query should be convertible to the binary_query");
    static_assert(ozo::TimeConstraint<TimeConstraint>, "should model TimeConstraint concept");
    async_get_connection(std::forward<P>(provider), deadline(t),
        async_request_op {
            std::forward<Q>(query),
            deadline(t),
            none,
            std::forward<Handler>(handler)
        }
    );
}

} // namespace ozo::impl
