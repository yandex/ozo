#pragma once

#include <ozo/asio.h>
#include <ozo/error.h>
#include <ozo/time_traits.h>
#include <ozo/connection.h>
#include <boost/asio/bind_executor.hpp>

namespace ozo::detail {

template <typename Socket, typename Executor, typename Allocator>
struct timeout_handler {
    Socket* socket = nullptr;
    Executor ex;
    Allocator a;

    timeout_handler(Socket& socket, const Executor& ex, const Allocator& a)
    : socket(std::addressof(socket)), ex(ex), a(a) {}

    void operator() (error_code ec) {
        if (ec != asio::error::operation_aborted) {
            socket->cancel(ec);
        }
    }
    using executor_type = Executor;

    executor_type get_executor() const noexcept { return ex;}

    using allocator_type = Allocator;

    allocator_type get_allocator() const noexcept { return a;}
};

template <typename T, typename Handler>
void set_io_timeout(T&& conn, const Handler& h, time_traits::duration t) {
    get_timer(conn).expires_after(t);
    get_timer(conn).async_wait(
        timeout_handler{
            get_socket(conn),
            asio::get_associated_executor(h),
            asio::get_associated_allocator(h)
        }
    );
}

} // namespace ozo::detail
