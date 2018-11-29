#pragma once

#include <ozo/asio.h>
#include <ozo/error.h>
#include <ozo/time_traits.h>
#include <ozo/connection.h>
#include <boost/asio/bind_executor.hpp>

namespace ozo::detail {

template <typename Socket, typename Executor>
struct timeout_handler {
    Socket* socket = nullptr;
    Executor ex;

    timeout_handler(Socket& socket, const Executor& ex)
    : socket(std::addressof(socket)), ex(ex) {}

    void operator() (error_code ec) {
        if (ec != asio::error::operation_aborted) {
            socket->cancel(ec);
        }
    }
    using executor_type = Executor;
    executor_type get_executor() const noexcept { return ex;}
};

template <typename T, typename Executor>
void set_io_timeout(T&& conn, const Executor& ex, time_traits::duration t) {
    get_timer(conn).expires_after(t);
    get_timer(conn).async_wait(timeout_handler(get_socket(conn), ex));
}

} // namespace ozo::detail
