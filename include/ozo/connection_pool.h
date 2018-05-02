#pragma once

#include <ozo/impl/async_connect.h>
#include <ozo/connection_info.h>
#include <ozo/connection.h>
#include <yamail/resource_pool/async/pool.hpp>

namespace ozo {
namespace impl {

template <typename ...Ts>
using connection_pool = yamail::resource_pool::async::pool<connection<Ts...>>;

template <typename ...Ts>
struct pooled_connection {
    using handle_type = typename connection_pool<Ts...>::handle;

    handle_type handle_;

    pooled_connection(handle_type&& handle) : handle_(std::move(handle)) {}

    decltype(auto) empty() const {return handle_.empty();}

    void reset(typename handle_type::value_type&& v) {
        handle_.reset(std::move(v));
    }

    ~pooled_connection() {
        if (connection_bad(*this)) {
            handle_.waste();
        }
    }

    friend auto& unwrap_connection(pooled_connection& conn) noexcept {
        return *(conn.handle_);
    }
    friend const auto& unwrap_connection(const pooled_connection& conn) noexcept {
        return *(conn.handle_);
    }
};
template <typename ...Ts>
using pooled_connection_ptr = std::shared_ptr<pooled_connection<Ts...>>;

template <typename P, typename OidMap, typename Statistics, typename Handler>
struct pooled_connection_wrapper {
    io_context& io_;
    P& provider_;
    Statistics statistics_;
    Handler handler_;

    template <typename Handle>
    void operator ()(error_code ec, Handle&& handle) {
        using boost::asio::asio_handler_invoke;

        if (ec) {
            return handler_(std::move(ec), pooled_connection_ptr<OidMap, Statistics>{});
        }

        auto conn = std::make_shared<pooled_connection<OidMap, Statistics>>(std::forward<Handle>(handle));
        if (!conn->empty() && connection_good(conn)) {
            return handler_(std::move(ec), std::move(conn));
        }

        conn->reset({io_, statistics_});
        get_connection(provider_, [h = std::move(handler_), pooled_conn = std::move(conn)] (error_code ec, auto conn) mutable {
            if (!ec) {
                pooled_conn->handle_.reset(std::move(unwrap_connection(std::move(conn))));
            }
            h(std::move(ec), std::move(pooled_conn));
        });
    }

    template <typename Func>
    friend void asio_handler_invoke(Func&& f, pooled_connection_wrapper* ctx) {
        using ::boost::asio::asio_handler_invoke;
        asio_handler_invoke(std::forward<Func>(f), std::addressof(ctx->handler_));
    }
};

template <typename P, typename OidMap, typename Statistics, typename Handler>
auto wrap_pooled_connection_handler(io_context& io, P& provider,
    Statistics&& stat, Handler&& handler) {
    static_assert(ConnectionProvider<P>, "is not a ConnectionProvider");
    return pooled_connection_wrapper<std::decay_t<P>, OidMap,
            std::decay_t<Statistics>, std::decay_t<Handler>>{
        io, provider, std::forward<Statistics>(stat),
        std::forward<Handler>(handler)};
}

} // namespace impl

static_assert(Connectable<impl::pooled_connection_ptr<empty_oid_map, no_statistics>>,
    "pooled_connection_ptr is not a Connectable concept");

static_assert(ConnectionProvider<impl::pooled_connection_ptr<empty_oid_map, no_statistics>>,
    "pooled_connection_ptr is not a ConnectionProvider concept");

template <typename P, typename OidMap, typename Statistics>
class connection_pool {
public:
    connection_pool(P provider, std::size_t capacity,
         std::size_t queue_capacity, Statistics statistics)
        : impl_(capacity, queue_capacity), provider_(std::move(provider)),
          statistics_(std::move(statistics)) {}

    using oid_map_type = OidMap;
    using statistics_type = Statistics;
    using duration = yamail::resource_pool::time_traits::duration;
    using time_point = yamail::resource_pool::time_traits::time_point;
    using connectable_type = impl::pooled_connection_ptr<oid_map_type, statistics_type>;

    template <typename Handler>
    void get_connection(io_context& io, duration timeout, Handler&& handler) {
        impl_.get_auto_recycle(io,
            impl::wrap_pooled_connection_handler<P, oid_map_type>(
                io, provider_, statistics_, std::forward<Handler>(handler)),
            timeout);
    }

private:
    impl::connection_pool<OidMap, Statistics> impl_;
    P provider_;
    Statistics statistics_;
};

template <typename T>
struct is_connection_pool : std::false_type {};

template <typename ...Args>
struct is_connection_pool<connection_pool<Args...>> : std::true_type {};

template <typename T>
constexpr auto ConnectionPool = is_connection_pool<std::decay_t<T>>::value;

template <
    typename P,
    typename OidMap = empty_oid_map,
    typename Statistics = no_statistics>
auto make_connection_pool(P&& provider, std::size_t capacity,
         std::size_t queue_capacity, OidMap&& = OidMap{},
        Statistics&& statistics = Statistics{}) {
    static_assert(ConnectionProvider<P>, "is not a ConnectionProvider");
    return connection_pool<std::decay_t<P>,
            std::decay_t<OidMap>, std::decay_t<Statistics>>{
        std::forward<P>(provider), capacity, queue_capacity,
        std::forward<Statistics>(statistics)};
}

template <typename Pool>
class connection_pool_provider {
public:
    connection_pool_provider(Pool& pool, io_context& io,
                             Require<ConnectionPool<Pool>, typename Pool::duration> timeout = Pool::duration::max())
        : pool_(pool), io_(io), timeout_(timeout) {}

    using oid_map_type = typename Pool::oid_map_type;
    using statistics_type = typename Pool::statistics_type;
    using connectable_type = typename Pool::connectable_type;

    template <typename Handler>
    friend void async_get_connection(connection_pool_provider& self, Handler&& h) {
        self.pool_.get_connection(self.io_, self.timeout_, std::forward<Handler>(h));
    }

private:
    Pool& pool_;
    io_context& io_;
    typename Pool::duration timeout_;
};

template <typename T>
inline auto make_provider(T& pool, io_context& io,
    Require<ConnectionPool<T>, typename T::duration> timeout = T::duration::max()) {
    static_assert(ConnectionPool<T>, "is not a ConnectionPool");
    return connection_pool_provider<std::decay_t<T>> {pool, io, timeout};
}

} // namespace ozo
