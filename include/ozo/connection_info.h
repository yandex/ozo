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
    void operator ()(io_context& io, Handler&& handler,
            time_traits::duration timeout = time_traits::duration::max()) const {
        impl::async_connect(
            conn_str,
            timeout,
            std::make_shared<connection>(io, statistics),
            std::forward<Handler>(handler)
        );
    }
};

/**
 * @brief Constructs `ozo::connection_info` #ConnectionSource.
 * @ingroup group-connection-functions
 * @relates ozo::connection_info
 *
 * @param conn_str --- standard libpq connection string.
 * @param OidMap --- oid map for user defined types.
 * @param statistics --- statistics to collect for a connection.
 * @return `ozo::connection_info` specialization.
 */
template <typename OidMap = empty_oid_map, typename Statistics = no_statistics>
inline auto make_connection_info(std::string conn_str, const OidMap& = OidMap{},
        Statistics statistics = Statistics{}) {
    return connection_info<OidMap, Statistics>{std::move(conn_str), statistics};
}

static_assert(ConnectionProvider<connector<connection_info<>>>, "is not a ConnectionProvider");

} // namespace ozo
