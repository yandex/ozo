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
        reset(std::move(v));
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

template <typename Connector, typename OidMap, typename Statistics, typename Handler>
struct pooled_connection_wrapper {
    io_context& io_;
    Connector connector_;
    Statistics statistics_;
    Handler handler_;

    template <typename Handle>
    void operator ()(error_code ec, Handle&& handle) {
        if (ec) {
            return handler_(std::move(ec), pooled_connection_ptr<OidMap, Statistics>{});
        }

        auto conn = std::make_shared<pooled_connection<OidMap, Statistics>>(std::move(handle));
        if (!conn->empty() && connection_good(conn)) {
            return handler_(std::move(ec), std::move(conn));
        }

        conn->reset({io_, statistics_});
        connector_(std::move(conn), std::move<Handler>(handler_));
    }

    template <typename Func>
    friend void asio_handler_invoke(Func&& f, pooled_connection_wrapper* ctx) {
        using ::boost::asio::asio_handler_invoke;
        asio_handler_invoke(std::forward<Func>(f), std::addressof(ctx->handler_));
    }
};

template <typename Connector, typename OidMap, typename Statistics, typename Handler>
auto wrap_pooled_connection_handler(io_context& io, Connector&& connector,
    Statistics&& stat, Handler&& handler) {
    return pooled_connection_wrapper<std::decay_t<Connector>, OidMap,
            std::decay_t<Statistics>, std::decay_t<Handler>>{
        io, std::forward<Connector>(connector), std::forward<Statistics>(stat),
        std::forward<Handler>(handler)};
}

} // namespace impl

struct async_connector {
    std::string conn_info_;
    template <typename T, typename Handler>
    Require<Connectable<T>> operator()(T&& conn, Handler&& handler) const {
        async_connect(conn_info_, std::forward<T>(conn),
            std::forward<Handler>(handler));
    }
};

// struct async_timed_connector {
//     std::string conn_info_;
//     std::chrono::steady_clock::duration timeout_;
//     template <typename T, typename Handler>
//     Require<Connectable<T>> operator()(T&& conn, Handler&& handler) const {
//         decltype(auto) socket = get_socket(conn);
//         async_connect(conn_info_, std::forward<T>(conn),
//             with_timeout(socket, timeout_, std::forward<Handler>(handler)));
//     }
// };

static_assert(Connectable<impl::pooled_connection_ptr<empty_oid_map, no_statistics>>,
    "pooled_connection_ptr is not a Connectable concept");

static_assert(ConnectionProvider<impl::pooled_connection_ptr<empty_oid_map, no_statistics>>,
    "pooled_connection_ptr is not a ConnectionProvider concept");

template <typename Connector, typename OidMap, typename Statistics>
class connection_pool {
public:
    connection_pool(Connector connector, std::size_t capacity,
         std::size_t queue_capacity, Statistics statistics)
    : impl_(capacity, queue_capacity), connector_(std::move(connector)),
      statistics_(std::move(statistics)) {}

    using oid_map_type = OidMap;
    using statistics_type = Statistics;
    using duration = yamail::resource_pool::time_traits::duration;
    using time_point = yamail::resource_pool::time_traits::time_point;

    template <typename Handler>
    void get_connection(io_context& io, duration timeout, Handler&& handler) {
        impl_.get_auto_recycle(io,
            impl::wrap_pooled_connection_handler(
                io, connector_, statistics_, std::forward<Handler>(handler)),
            timeout);
    }

private:
    impl::connection_pool<OidMap, Statistics> impl_;
    Connector connector_;
    Statistics statistics_;
};

template <typename T>
struct is_connection_pool : std::false_type {};

template <typename ...Args>
struct is_connection_pool<connection_pool<Args...>> : std::true_type {};

template <typename T>
constexpr auto ConnectionPool = is_connection_pool<std::decay_t<T>>::value;

template <
    typename Connector = async_connector,
    typename OidMap = empty_oid_map,
    typename Statistics = no_statistics>
auto make_connection_pool(Connector&& connector, std::size_t capacity,
         std::size_t queue_capacity, OidMap&& = OidMap{},
        Statistics&& statistics = Statistics{}) {
    return connection_pool<std::decay_t<Connector>,
            std::decay_t<OidMap>, std::decay_t<Statistics>>{
        std::forward<Connector>(connector), capacity, queue_capacity,
        std::forward<Statistics>(statistics)};
}

template <typename T>
inline auto make_provider(T& pool, io_context& io,
    Require<ConnectionPool<T>, typename T::duration> timeout = T::time_point::max()) {

    return [&pool, &io, timeout](auto handler) {
        pool.get_connection(io, timeout, std::move(handler));
    };
}

} // namespace ozo
