#pragma once

#include <ozo/impl/connection_pool.h>
#include <ozo/connection_info.h>
#include <ozo/asio.h>

namespace ozo {

struct connection_pool_config {
    std::size_t capacity = 10;
    std::size_t queue_capacity = 128;
};

template <typename Source>
class connection_pool {
public:
    connection_pool(Source source, const connection_pool_config& config)
    : impl_(config.capacity, config.queue_capacity), source_(std::move(source)) {}

    using duration = yamail::resource_pool::time_traits::duration;
    using time_point = yamail::resource_pool::time_traits::time_point;
    using connection_type = impl::pooled_connection_ptr<Source>;

    template <typename Handler>
    void get_connection(io_context& io, duration timeout, Handler&& handler) {
        impl_.get_auto_recycle(
            io,
            impl::wrap_pooled_connection_handler(io, make_connector(source_, io), std::forward<Handler>(handler)),
            timeout
        );
    }

private:
    impl::connection_pool<Source> impl_;
    Source source_;
};

static_assert(ConnectionSource<connection_pool<connection_info<>>>, "is not a ConnectionSource");

template <typename Source>
class connection_pool_connector {
public:
    using base_type = connection_pool<Source>;
    using duration = typename base_type::duration;
    using connection_type = typename connection_pool<Source>::connection_type;

    connection_pool_connector(base_type& base, io_context& io, duration timeout)
            : base(base), io(io), timeout(timeout) {}

    template <typename Handler>
    void async_get_connection(Handler&& handler) {
        base.get_connection(io, timeout, std::forward<Handler>(handler));
    }

private:
    base_type& base;
    io_context& io;
    duration timeout;
};

static_assert(ConnectionProvider<connection_pool_connector<connection_info<>>>, "is not a ConnectionProvider");

template <typename Source>
auto make_connector(connection_pool<Source>& base, io_context& io,
        typename connection_pool_connector<Source>::duration timeout = connection_pool_connector<Source>::duration::max()) {
    return connection_pool_connector<Source>(base, io, timeout);
}

template <typename T>
struct is_connection_pool : std::false_type {};

template <typename ...Args>
struct is_connection_pool<connection_pool<Args...>> : std::true_type {};

template <typename T>
constexpr auto ConnectionPool = is_connection_pool<std::decay_t<T>>::value;

template <typename Source>
auto make_connection_pool(Source&& source, const connection_pool_config& config) {
    static_assert(ConnectionSource<Source>, "is not a ConnectionSource");
    return connection_pool<std::decay_t<Source>>{std::forward<Source>(source), config};
}

} // namespace ozo
