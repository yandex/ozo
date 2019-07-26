#pragma once

#include <ozo/impl/io.h>
#include <ozo/connection.h>
#include <ozo/type_traits.h>
#include <ozo/time_traits.h>
#include <ozo/detail/deadline.h>
#include <ozo/detail/make_copyable.h>
#include <ozo/detail/call_once.h>

#include <boost/asio/bind_executor.hpp>
#include <future>

namespace ozo {

namespace impl {

template <typename T>
inline bool pq_cancel(cancel_handle<T> h, std::string& err) {
    return PQcancel(h.native_handle(), std::data(err), std::size(err));
}

template <typename Handle>
inline auto dispatch_cancel(Handle&& h) {
    auto res = std::make_tuple(error_code{}, std::string(255, '\0'));
    if (!pq_cancel(std::forward<Handle>(h), std::get<std::string>(res))) {
        std::get<error_code>(res) = error::pq_cancel_failed;
        auto& msg = std::get<std::string>(res);
        const auto pos = msg.find_last_not_of('\0');
        if (pos == msg.npos) {
            msg.clear();
        } else if (pos < msg.size()) {
            msg.resize(pos + 1);
        }
    } else {
        std::get<std::string>(res).clear();
    }
    return res;
}

template <typename CancelHandler>
struct on_cancel_op_timer {

    CancelHandler handler_;

    on_cancel_op_timer(CancelHandler handler) : handler_(handler) {}

    void operator() (error_code) {
        handler_(asio::error::timed_out, "cancel() operation waiting aborted by time-out");
    }

    using executor_type = asio::associated_executor_t<CancelHandler>;

    executor_type get_executor() const noexcept { return asio::get_associated_executor(handler_);}

    using allocator_type = asio::associated_allocator_t<CancelHandler>;

    allocator_type get_allocator() const noexcept { return asio::get_associated_allocator(handler_);}
};

template <typename Handle, typename CancelHandler>
struct cancel_op {
    CancelHandler handler_;
    Handle cancel_handle_;

    cancel_op(Handle cancel_handle, CancelHandler handler)
    : handler_(std::move(handler)), cancel_handle_(std::move(cancel_handle))
    {}

    using executor_type = typename Handle::executor_type;

    executor_type get_executor() const { return cancel_handle_.get_executor();}

    using allocator_type = typename asio::associated_allocator<CancelHandler>::type;

    allocator_type get_allocator() const noexcept { return asio::get_associated_allocator(handler_);}

    void operator () () {
        auto [ec, msg] = dispatch_cancel(std::move(cancel_handle_));
        asio::dispatch(detail::bind(std::move(handler_), std::move(ec), std::move(msg)));
    }
};

struct initiate_async_cancel {
    template <typename CompletionHandler, typename Handle, typename IoContext>
    inline auto operator () (CompletionHandler&& h, Handle&& cancel_handle, IoContext& io, time_traits::time_point t) const {
        using shared_handler_t = detail::make_copyable<detail::call_once<CompletionHandler>>;
        auto allocator = asio::get_associated_allocator(h);
        shared_handler_t shared_handler(std::allocator_arg, allocator, std::forward<CompletionHandler>(h));
        asio::post(cancel_op{
            std::forward<Handle>(cancel_handle),
            detail::deadline_handler(io.get_executor(), t, shared_handler, on_cancel_op_timer{shared_handler})
        });
    }

    template <typename CompletionHandler, typename Handle>
    inline auto operator () (CompletionHandler&& h, Handle&& cancel_handle) const {
        asio::post(cancel_op{
            std::forward<Handle>(cancel_handle),
            std::move(h)
        });
    }
};

} // namespace impl

template <typename Connection, typename Executor>
inline cancel_handle<Executor> get_cancel_handle(const Connection& connection, Executor&& executor) {
    static_assert(ozo::Connection<Connection>, "First argument should model a Connection");
    return {PQgetCancel(get_native_handle(connection)), std::forward<Executor>(executor)};
}

using cancel_handler_signature_t = void (error_code, std::string);

template <typename Executor, typename TimeConstraint, typename CompletionToken>
auto cancel_op::operator() (cancel_handle<Executor>&& handle, io_context& io,
        TimeConstraint time_constraint, CompletionToken&& token) const {
    static_assert(ozo::TimeConstraint<TimeConstraint>, "time_constraint should model TimeConstrain");
    return async_initiate<CompletionToken, cancel_handler_signature_t>(
        impl::initiate_async_cancel{}, token, std::move(handle), io, deadline(time_constraint)
    );
}

template <typename Executor, typename CompletionToken>
auto cancel_op::operator() (cancel_handle<Executor>&& handle, CompletionToken&& token) const {
    return async_initiate<CompletionToken, cancel_handler_signature_t>(
        impl::initiate_async_cancel{}, token, std::move(handle)
    );
}

} // namespace ozo
