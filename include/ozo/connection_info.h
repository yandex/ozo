#pragma once

#include <ozo/connection.h>
#include <ozo/impl/async_connect.h>

#include <chrono>

namespace ozo {

template <
    typename OidMap = empty_oid_map,
    typename Statistics = no_statistics>
class connection_info {
    io_context& io_;
    std::string conn_str_;
    Statistics statistics_;

    using connection = impl::connection_impl<OidMap, Statistics>;

public:
    using connection_type = std::shared_ptr<connection>;

    connection_info(io_context& io, std::string conn_str,
            Statistics statistics = Statistics{})
    : io_(io), conn_str_(std::move(conn_str)), statistics_(std::move(statistics)) {}

    template <typename Handler>
    void async_get_connection(Handler&& h) const {
        impl::async_connect(conn_str_,
            std::make_shared<connection>(io_, statistics_),
            std::forward<Handler>(h));
    }
};

static_assert(
    ConnectionProvider<connection_info<>>,
    "connection_info does not fit ConnectionProvider concept");

} // namespace ozo
