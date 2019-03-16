#pragma once

#include <ozo/connection.h>
#include <ozo/transaction_status.h>
#include <yamail/resource_pool/async/pool.hpp>
#include <ozo/asio.h>
#include <ozo/ext/std/shared_ptr.h>

namespace ozo::impl {

template <typename Source>
struct get_connection_pool {
    using type = yamail::resource_pool::async::pool<connection_type<Source>>;
};

template <typename Source>
using connection_pool = typename get_connection_pool<Source>::type;

template <typename Source>
struct pooled_connection {
    using handle_type = typename connection_pool<Source>::handle;
    using underlying_type = typename handle_type::value_type;

    handle_type handle_;

    pooled_connection(handle_type&& handle) : handle_(std::move(handle)) {}

    bool empty() const {return handle_.empty();}

    void reset(underlying_type&& v) {
        handle_.reset(std::move(v));
    }

    ~pooled_connection() {
        if (!empty() && (connection_bad(*this)
                || get_transaction_status(*this) != transaction_status::idle)) {
            handle_.waste();
        }
    }
};
template <typename Source>
using pooled_connection_ptr = std::shared_ptr<pooled_connection<Source>>;

} // namespace ozo::impl
namespace ozo {
template <typename T>
struct unwrap_impl<impl::pooled_connection<T>> {
    template <typename Conn>
    static constexpr decltype(auto) apply(Conn&& conn) noexcept {
        return unwrap(*conn.handle_);
    }
};

template <typename T>
struct is_nullable<impl::pooled_connection<T>> : std::true_type {};

template <typename T>
struct is_null_impl<impl::pooled_connection<T>> {
    static bool apply(const impl::pooled_connection<T>& conn) {
        return conn.empty() ? true : is_null(unwrap(conn));
    }
};
} // namespace ozo

namespace ozo::impl {

template <typename IoContext, typename Provider, typename Handler>
struct pooled_connection_wrapper {
    IoContext& io_;
    Provider provider_;
    Handler handler_;

    using connection = pooled_connection<typename Provider::source_type>;
    using connection_ptr = pooled_connection_ptr<typename Provider::source_type>;

    struct wrapper {
        Handler handler_;
        connection_ptr conn_;

        template <typename Conn>
        void operator () (error_code ec, Conn&& conn) {
            static_assert(std::is_same_v<connection_type<Provider>, std::decay_t<Conn>>,
                "Conn must connectiable type of Provider");
            if (conn) {
                conn_->reset(std::move(conn));
            } else {
                conn_ = nullptr;
            }
            handler_(std::move(ec), std::move(conn_));
        }

        using executor_type = decltype(asio::get_associated_executor(handler_));

        executor_type get_executor() const noexcept {
            return asio::get_associated_executor(handler_);
        }

        using allocator_type = decltype(asio::get_associated_allocator(handler_));

        allocator_type get_allocator() const noexcept {
            return asio::get_associated_allocator(handler_);
        }
    };

    template <typename Handle>
    void operator ()(error_code ec, Handle&& handle) {
        if (ec) {
            return handler_(std::move(ec), connection_ptr{});
        }

        auto conn = std::allocate_shared<connection>(get_allocator(), std::forward<Handle>(handle));
        if (connection_good(conn)) {
            ec = rebind_io_context(conn, io_);
            return handler_(std::move(ec), std::move(conn));
        }

        async_get_connection(provider_, wrapper{std::move(handler_), std::move(conn)});
    }

    using executor_type = decltype(asio::get_associated_executor(handler_));

    executor_type get_executor() const noexcept {
        return asio::get_associated_executor(handler_);
    }

    using allocator_type = decltype(asio::get_associated_allocator(handler_));

    allocator_type get_allocator() const noexcept {
        return asio::get_associated_allocator(handler_);
    }
};

template <typename P, typename IoContext, typename Handler>
auto wrap_pooled_connection_handler(IoContext& io, P&& provider, Handler&& handler) {

    static_assert(ConnectionProvider<P>, "is not a ConnectionProvider");

    return pooled_connection_wrapper<IoContext, std::decay_t<P>, std::decay_t<Handler>> {
        io, std::forward<P>(provider), std::forward<Handler>(handler)
    };
}

static_assert(Connection<pooled_connection_ptr<connection_impl<empty_oid_map, no_statistics>>>,
    "pooled_connection_ptr is not a Connection concept");

static_assert(ConnectionProvider<pooled_connection_ptr<connection_impl<empty_oid_map, no_statistics>>>,
    "pooled_connection_ptr is not a ConnectionProvider concept");

} // namespace ozo::impl
