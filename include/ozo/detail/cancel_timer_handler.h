#pragma once

#include <ozo/asio.h>
#include <ozo/error.h>

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

} // namespace ozo::detail
