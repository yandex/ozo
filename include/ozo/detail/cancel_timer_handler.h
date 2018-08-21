#pragma once

#include <ozo/asio.h>
#include <ozo/error.h>

namespace ozo::detail {

template <typename Handler>
struct cancel_timer_handler {
    Handler handler;

    template <typename Connection>
    void operator() (error_code ec, Connection&& connection) {
        get_timer(connection).cancel();
        handler(ec, std::forward<Connection>(connection));
    }

    using executor_type = decltype(asio::get_associated_executor(handler));

    auto get_executor() const noexcept {
        return asio::get_associated_executor(handler);
    }
};

template <typename Handler>
inline auto make_cancel_timer_handler(Handler&& handler) {
    return cancel_timer_handler<std::decay_t<Handler>> {std::forward<Handler>(handler)};
}

} // namespace ozo::detail
