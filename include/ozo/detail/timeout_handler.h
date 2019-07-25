#pragma once

#include <ozo/asio.h>
#include <ozo/error.h>

namespace ozo::detail {

template <typename Socket, typename Allocator>
struct cancel_socket {
    Socket& socket_;
    Allocator allocator_;

    cancel_socket(Socket& socket, const Allocator& allocator)
    : socket_(socket), allocator_(allocator) {}

    void operator() (error_code ec) const {
        socket_.cancel(ec);
    }

    using allocator_type = Allocator;

    allocator_type get_allocator() const noexcept { return allocator_;}
};

} // namespace ozo::detail
