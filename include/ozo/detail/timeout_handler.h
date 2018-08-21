#pragma once

#include <ozo/asio.h>
#include <ozo/error.h>

namespace ozo::detail {

template <typename Socket>
struct timeout_handler {
    Socket& socket;

    void operator() (error_code ec) {
        if (ec != asio::error::operation_aborted) {
            socket.cancel(ec);
        }
    }
};

template <typename Socket>
auto make_timeout_handler(Socket& socket) {
    return timeout_handler<std::decay_t<Socket>> {socket};
}

} // namespace ozo::detail
