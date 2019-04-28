#pragma once

#include <ozo/asio.h>
#include <ozo/error.h>
#include <ozo/time_traits.h>
#include <ozo/deadline.h>

namespace ozo::detail {

template <typename Handler>
struct cancel_timer_handler {
    Handler handler;

    cancel_timer_handler(Handler handler) : handler{std::move(handler)} {}

    template <typename Connection>
    void operator() (error_code ec, Connection&& connection) {
        get_timer(connection).cancel();
        handler(ec, std::forward<Connection>(connection));
    }

    using executor_type = decltype(asio::get_associated_executor(handler));

    executor_type get_executor() const noexcept {
        return asio::get_associated_executor(handler);
    }

    using allocator_type = decltype(asio::get_associated_allocator(handler));

    allocator_type get_allocator() const noexcept {
        return asio::get_associated_allocator(handler);
    }
};

template <typename T>
struct bind_cancel_timer_impl {
    template <typename Handler>
    constexpr static decltype(auto) apply(Handler&& h) {
        return detail::cancel_timer_handler(std::forward<Handler>(h));
    }
};

template <>
struct bind_cancel_timer_impl<ozo::no_time_constrain_t> {
    template <typename Handler>
    constexpr static decltype(auto) apply(Handler&& h) {
        return std::forward<Handler>(h);
    }
};

template <typename TimeConstrain, typename Handler>
inline constexpr decltype(auto) bind_cancel_timer(Handler&& h) {
    return bind_cancel_timer_impl<TimeConstrain>::apply(std::forward<Handler>(h));
}

} // namespace ozo::detail
