#pragma once

#include <ozo/connection.h>
#include <yamail/resource_pool/async/pool.hpp>
#include <ozo/asio.h>
#include <ozo/ext/std/shared_ptr.h>
#include <ozo/detail/make_copyable.h>


namespace ozo::detail {

template <typename Allocator, typename Executor, typename Rep>
auto create_pooled_connection(const Allocator& alloc, const Executor& ex, Rep&& rep) {
    return std::allocate_shared<pooled_connection<std::decay_t<Rep>, Executor>>(alloc, ex, std::forward<Rep>(rep));
}

template <typename Source, typename Handler, typename TimeConstraint>
struct pooled_connection_wrapper {
    using connection_ptr = typename connection_pool<Source>::connection_type;
    using connection = typename connection_ptr::element_type;
    using handle_type = typename connection::rep_type;

    typename connection::executor_type io_executor_;
    Source source_;
    detail::make_copyable_t<Handler> handler_;
    TimeConstraint time_constrain_;

    struct wrapper {
        Handler handler_;
        handle_type handle_;

        template <typename Conn>
        void operator () (error_code ec, Conn&& conn) {
            static_assert(std::is_same_v<connection_type<Source>, std::decay_t<Conn>>,
                "Conn should be connection type of Source");
            if (!is_null(conn)) {
                auto& target = ozo::unwrap_connection(conn);

                handle_.reset({target.release(), target.oid_map(), target.get_error_context()});
                auto res = create_pooled_connection(
                    get_allocator(), target.get_executor(), std::move(handle_)
                );

                handler_(std::move(ec), std::move(res));
            } else {
                handler_(std::move(ec), connection_ptr{});
            }
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

    void operator ()(error_code ec, handle_type&& handle) {
        if (ec) {
            return handler_(std::move(ec), connection_ptr{});
        }

        if (!handle.empty() && !connection_status_bad(handle->safe_native_handle().get())) {
            auto conn = create_pooled_connection(get_allocator(), io_executor_, std::move(handle));
            return handler_(std::move(ec), std::move(conn));
        }

        source_(io_executor_.context(), time_constrain_, wrapper{std::move(handler_), std::move(handle)});
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

template <typename Source, typename Executor, typename TimeConstraint, typename Handler>
auto wrap_pooled_connection_handler(const Executor& ex, Source&& source, TimeConstraint t, Handler&& handler) {
    static_assert(ConnectionSource<Source>, "is not a ConnectionSource");

    return pooled_connection_wrapper<std::decay_t<Source>, std::decay_t<Handler>, TimeConstraint> {
        ex, std::forward<Source>(source), std::forward<Handler>(handler), t
    };
}

} // namespace ozo::detail

namespace ozo {

template <typename V>
struct unwrap_impl<yamail::resource_pool::handle<V>> {
    template <typename T>
    static constexpr decltype(auto) apply(T&& handle) {
        return *handle;
    }
};

template <typename Source, typename ThreadSafety>
template <typename TimeConstraint, typename Handler>
void connection_pool<Source, ThreadSafety>::operator ()(io_context& io, TimeConstraint t, Handler&& handler) {
    static_assert(ozo::TimeConstraint<TimeConstraint>, "should model TimeConstraint concept");
    impl_.get_auto_recycle(
        io,
        detail::wrap_pooled_connection_handler(
            io.get_executor(),
            source_,
            t,
            std::forward<Handler>(handler)
        ),
        queue_timeout(t)
    );
}

template <typename Rep, typename Executor>
pooled_connection<Rep, Executor>::pooled_connection(const Executor& ex, Rep&& rep)
: rep_(std::move(rep)), ex_(ex), stream_(get_executor().context()) {
    if (auto fd = PQsocket(native_handle()); fd != -1) {
        stream_.assign(fd);
    }
}

template <typename Rep, typename Executor>
typename pooled_connection<Rep, Executor>::native_handle_type
pooled_connection<Rep, Executor>::native_handle() const noexcept {
    if (rep_.empty()) {
        return {};
    }
    return ozo::unwrap(rep_).safe_native_handle().get();
}

template <typename Rep, typename Executor>
template <typename WaitHandler>
void pooled_connection<Rep, Executor>::async_wait_write(WaitHandler&& h) {
    stream_.async_write_some(asio::null_buffers(), std::forward<WaitHandler>(h));
}

template <typename Rep, typename Executor>
template <typename WaitHandler>
void pooled_connection<Rep, Executor>::async_wait_read(WaitHandler&& h) {
    stream_.async_read_some(asio::null_buffers(), std::forward<WaitHandler>(h));
}

template <typename Rep, typename Executor>
error_code pooled_connection<Rep, Executor>::close() noexcept {
    stream_.release();
    ozo::unwrap(rep_).safe_native_handle().reset();
    return error_code{};
}

template <typename Rep, typename Executor>
void pooled_connection<Rep, Executor>::cancel() noexcept {
    error_code _;
    stream_.cancel(_);
}

template <typename Rep, typename Executor>
bool pooled_connection<Rep, Executor>::is_bad() const noexcept {
    return !detail::connection_status_ok(native_handle());
}

template <typename Rep, typename Executor>
pooled_connection<Rep, Executor>::~pooled_connection() {
    stream_.release();
    if (!rep_.empty() && (is_bad() || get_transaction_status(*this) != transaction_status::idle)) {
        rep_.waste();
    }
}

} // namespace ozo
