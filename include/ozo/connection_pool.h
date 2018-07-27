#pragma once

#include <ozo/impl/connection_pool.h>
#include <ozo/connection_info.h>
#include <ozo/asio.h>

namespace ozo {

struct connection_pool_config {
    std::size_t capacity = 10;
    std::size_t queue_capacity = 128;
};

template <typename P>
class connection_pool {
public:
    static_assert(ConnectionProvider<P>, "P must be ConnectionProvider");

    connection_pool(P provider, const connection_pool_config& config)
    : impl_(config.capacity, config.queue_capacity), provider_(std::move(provider)) {}

    using duration = yamail::resource_pool::time_traits::duration;
    using time_point = yamail::resource_pool::time_traits::time_point;
    using connection_type = impl::pooled_connection_ptr<P>;

    template <typename Handler>
    void get_connection(io_context& io, duration timeout, Handler&& handler) {
        impl_.get_auto_recycle(io,
            impl::wrap_pooled_connection_handler(io, provider_, std::forward<Handler>(handler)),
            timeout);
    }

private:
    impl::connection_pool<P> impl_;
    P provider_;
};

template <typename T>
struct is_connection_pool : std::false_type {};

template <typename ...Args>
struct is_connection_pool<connection_pool<Args...>> : std::true_type {};

template <typename T>
constexpr auto ConnectionPool = is_connection_pool<std::decay_t<T>>::value;

template <typename P>
auto make_connection_pool(P&& provider, const connection_pool_config& config) {
    static_assert(ConnectionProvider<P>, "is not a ConnectionProvider");
    return connection_pool<std::decay_t<P>>{std::forward<P>(provider), config};
}

template <typename Pool>
class connection_pool_provider {
public:
    static_assert(ConnectionPool<Pool>, "Pool must be ConnectionPool");

    using duration = typename Pool::duration;
    using connection_type = typename Pool::connection_type;

    connection_pool_provider(Pool& pool, io_context& io, duration timeout)
    : pool_(pool), io_(io), timeout_(timeout) {}

    connection_pool_provider(Pool& pool, io_context& io)
    : connection_pool_provider(pool, io, duration::max()) {}

    template <typename Handler>
    void async_get_connection(Handler&& h) {
        pool_.get_connection(io_, timeout_, std::forward<Handler>(h));
    }

private:
    Pool& pool_;
    io_context& io_;
    duration timeout_;
};

template <typename T>
inline auto make_provider(connection_pool<T>& pool, io_context& io,
    typename connection_pool<T>::duration timeout = connection_pool<T>::duration::max()) {
    return connection_pool_provider<connection_pool<T>> {pool, io, timeout};
}

} // namespace ozo
