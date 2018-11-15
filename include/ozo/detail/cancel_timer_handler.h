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
};

} // namespace ozo::detail
