#pragma once

#include <ozo/asio.h>
#include <ozo/error.h>

namespace ozo::detail {

template <typename Socket>
struct timeout_handler {
    Socket* socket = nullptr;

    timeout_handler(Socket& socket) : socket(std::addressof(socket)) {}

    void operator() (error_code ec) {
        if (ec != asio::error::operation_aborted) {
            socket->cancel(ec);
        }
    }
};

} // namespace ozo::detail
