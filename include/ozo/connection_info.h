#pragma once

#include <ozo/connection.h>
#include <ozo/async_connect.h>

#include <chrono>

namespace ozo {

template <
    typename OidMap = empty_oid_map, 
    typename Statistics = no_statistics>
class connection_info {
    io_context& io_;
    std::string conn_str_;
    Statistics statistics_;

    using connection = impl::connection<OidMap, Statistics>;

public:
    using connectiable_type = std::shared_ptr<connection>;

    connection_info(io_context& io, std::string conn_str,
            Statistics statistics = Statistics{})
    : io_(io), conn_str_(std::move(conn_str)), statistics_(std::move(statistics)) {}

    template <typename Handler>
    friend void async_get_connection(const connection_info& self, Handler&& h) {
        async_connect(self.conn_str_,
            std::make_shared<connection>(self.io_, self.statistics_),
            std::forward<Handler>(h));
    }
};

static_assert(
    ConnectionProvider<connection_info<>>,
    "connection_info does not fit ConnectionProvider concept");

} // namespace ozo
