#pragma once

#include <ozo/impl/async_connect.h>
#include <ozo/connection_info.h>
#include <ozo/connection.h>
#include <yamail/resource_pool/async/pool.hpp>

namespace ozo {
namespace impl {

template <typename Provider>
struct get_connection_pool {
    using type = yamail::resource_pool::async::pool<
        std::decay_t<decltype(unwrap_connection(std::declval<connectable_type<Provider>&>()))>
    >;
};

template <typename Provider>
using connection_pool = typename get_connection_pool<Provider>::type;

template <typename Provider>
struct pooled_connection {
    using handle_type = typename connection_pool<Provider>::handle;
    using underlying_type = typename handle_type::value_type;

    handle_type handle_;

    pooled_connection(handle_type&& handle) : handle_(std::move(handle)) {}

    bool empty() const {return handle_.empty();}

    void reset(underlying_type&& v) {
        handle_.reset(std::move(v));
    }

    ~pooled_connection() {
        if (!empty() && connection_bad(*this)) {
            handle_.waste();
        }
    }

    friend auto& unwrap_connection(pooled_connection& conn) noexcept {
        return unwrap_connection(*(conn.handle_));
    }
    friend const auto& unwrap_connection(const pooled_connection& conn) noexcept {
        return unwrap_connection(*(conn.handle_));
    }
};
template <typename Provider>
using pooled_connection_ptr = std::shared_ptr<pooled_connection<Provider>>;

template <typename Provider, typename Handler>
struct pooled_connection_wrapper {
    Provider provider_;
    Handler handler_;

    using connection = pooled_connection<Provider>;
    using connection_ptr = pooled_connection_ptr<Provider>;

    struct wrapper {
        Handler handler_;
        connection_ptr conn_;

        template <typename Conn>
        void operator () (error_code ec, Conn&& conn) {
            static_assert(std::is_same_v<connectable_type<Provider>, std::decay_t<Conn>>,
                "Conn must connectiable type of Provider");
            if (!ec) {
                conn_->reset(std::move(unwrap_connection(conn)));
            }
            handler_(std::move(ec), std::move(conn_));
        }

        template <typename Func>
        friend void asio_handler_invoke(Func&& f, wrapper* ctx) {
            using ::boost::asio::asio_handler_invoke;
            asio_handler_invoke(std::forward<Func>(f), std::addressof(ctx->handler_));
        }
    };

    template <typename Handle>
    void operator ()(error_code ec, Handle&& handle) {
        if (ec) {
            return handler_(std::move(ec), connection_ptr{});
        }

        auto conn = std::make_shared<connection>(std::forward<Handle>(handle));
        if (!conn->empty() && connection_good(conn)) {
            return handler_(std::move(ec), std::move(conn));
        }

        async_get_connection(provider_, wrapper{std::move(handler_), std::move(conn)});
    }

    template <typename Func>
    friend void asio_handler_invoke(Func&& f, pooled_connection_wrapper* ctx) {
        using ::boost::asio::asio_handler_invoke;
        asio_handler_invoke(std::forward<Func>(f), std::addressof(ctx->handler_));
    }
};

template <typename P, typename Handler>
auto wrap_pooled_connection_handler(P&& provider, Handler&& handler) {

    static_assert(ConnectionProvider<P>, "is not a ConnectionProvider");

    return pooled_connection_wrapper<std::decay_t<P>, std::decay_t<Handler>> {
        std::forward<P>(provider), std::forward<Handler>(handler)
    };
}

} // namespace impl

static_assert(Connectable<impl::pooled_connection_ptr<impl::connection<empty_oid_map, no_statistics>>>,
    "pooled_connection_ptr is not a Connectable concept");

static_assert(ConnectionProvider<impl::pooled_connection_ptr<impl::connection<empty_oid_map, no_statistics>>>,
    "pooled_connection_ptr is not a ConnectionProvider concept");

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
    using connectable_type = impl::pooled_connection_ptr<P>;

    template <typename Handler>
    void get_connection(io_context& io, duration timeout, Handler&& handler) {
        impl_.get_auto_recycle(io,
            impl::wrap_pooled_connection_handler(provider_, std::forward<Handler>(handler)),
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
    using connectable_type = typename Pool::connectable_type;

    connection_pool_provider(Pool& pool, io_context& io, duration timeout)
    : pool_(pool), io_(io), timeout_(timeout) {}

    connection_pool_provider(Pool& pool, io_context& io)
    : connection_pool_provider(pool, io, duration::max()) {}

    template <typename Handler>
    void operator() (Handler&& h) {
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
