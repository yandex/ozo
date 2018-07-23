#pragma once

#include <ozo/connection.h>
#include <ozo/impl/async_connect.h>

#include <chrono>

namespace ozo {

template <
    typename OidMap = empty_oid_map,
    typename Statistics = no_statistics>
class connection_info {
    std::string conn_str_;
    Statistics statistics_;

    using connection = impl::connection<OidMap, Statistics>;

public:
    using connectable_type = std::shared_ptr<connection>;

    connection_info(std::string conn_str, Statistics statistics = Statistics{})
    : conn_str_(std::move(conn_str)), statistics_(std::move(statistics)) {}

    template <typename Handler>
    friend void async_get_connection(const connection_info& self, io_context& io, Handler&& h) {
        impl::async_connect(self.conn_str_,
            std::make_shared<connection>(io, self.statistics_),
            std::forward<Handler>(h));
    }
};

static_assert(
    ConnectionProvider<connection_info<>>,
    "connection_info does not fit ConnectionProvider concept");

} // namespace ozo
