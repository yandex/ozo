#pragma once

#include <ozo/impl/connection_pool.h>
#include <ozo/connection_info.h>
#include <ozo/asio.h>

namespace ozo {

struct connection_pool_config {
    std::size_t capacity = 10;
    std::size_t queue_capacity = 128;
    time_traits::duration idle_timeout = std::chrono::seconds(60);
};

struct connection_pool_timeouts {
    time_traits::duration connect = std::chrono::seconds(10);
    time_traits::duration queue = std::chrono::seconds(10);
};

template <typename Source>
class connection_pool {
public:
    connection_pool(Source source, const connection_pool_config& config)
    : impl_(config.capacity, config.queue_capacity, config.idle_timeout),
      source_(std::move(source)) {}

    using connection_type = impl::pooled_connection_ptr<Source>;

    template <typename Handler>
    void operator ()(io_context& io, Handler&& handler,
            const connection_pool_timeouts& timeouts = connection_pool_timeouts {}) {
        impl_.get_auto_recycle(
            io,
            impl::wrap_pooled_connection_handler(
                io,
                make_connector(source_, io, timeouts.connect),
                std::forward<Handler>(handler)
            ),
            timeouts.queue
        );
    }

private:
    impl::connection_pool<Source> impl_;
    Source source_;
};

static_assert(ConnectionProvider<connector<connection_pool<connection_info<>>>>, "is not a ConnectionProvider");

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
