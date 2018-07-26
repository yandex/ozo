#pragma once

#include <ozo/connector.h>
#include <ozo/connection.h>
#include <ozo/impl/async_connect.h>

#include <chrono>

namespace ozo {

template <
    typename OidMap = empty_oid_map,
    typename Statistics = no_statistics>
class connection_info {
    std::string conn_str;
    Statistics statistics;

public:
    using connection = impl::connection_impl<OidMap, Statistics>;
    using connection_type = std::shared_ptr<connection>;

    connection_info(std::string conn_str, Statistics statistics = Statistics{})
            : conn_str(std::move(conn_str)), statistics(std::move(statistics)) {
    }

    template <typename Handler>
    void operator ()(io_context& io, Handler&& handler) const {
        impl::async_connect(
            conn_str,
            std::make_shared<connection>(io, statistics),
            std::forward<Handler>(handler)
        );
    }
};

static_assert(ConnectionProvider<connector<connection_info<>>>, "is not a ConnectionProvider");

} // namespace ozo
