#pragma once

#include <ozo/connection.h>
#include <ozo/impl/async_connect.h>

#include <chrono>

namespace ozo {

template <
    typename OidMap = empty_oid_map,
    typename Statistics = no_statistics>
struct connection_info {
    using connection = impl::connection_impl<OidMap, Statistics>;
    using connection_type = std::shared_ptr<connection>;

    std::string conn_str;
    Statistics statistics;

    connection_info(std::string conn_str, Statistics statistics = Statistics{})
            : conn_str(std::move(conn_str)), statistics(std::move(statistics)) {}
};

static_assert(ConnectionSource<connection_info<>>, "is not a ConnectionSource");

template <typename OidMap = empty_oid_map, typename Statistics = no_statistics>
class connection_info_connector {
public:
    using source_type = connection_info<OidMap, Statistics>;
    using connection = typename source_type::connection;
    using connection_type = typename source_type::connection_type;

    connection_info_connector(const source_type& base, io_context& io)
            : base(base), io(io) {}

    template <typename Handler>
    void async_get_connection(Handler&& handler) const {
        impl::async_connect(
            base.conn_str,
            std::make_shared<connection>(io, base.statistics),
            std::forward<Handler>(handler)
        );
    }

private:
    const source_type& base;
    io_context& io;
};

static_assert(ConnectionProvider<connection_info_connector<>>, "is not a ConnectionProvider");

template <typename OidMap, typename Statistics>
auto make_connector(const connection_info<OidMap, Statistics>& base, io_context& io) {
    return connection_info_connector<OidMap, Statistics>(base, io);
}

} // namespace ozo
