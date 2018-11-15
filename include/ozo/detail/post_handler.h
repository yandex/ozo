#pragma once

#include <ozo/asio.h>
#include <ozo/error.h>
#include <ozo/connection.h>

#include <boost/asio/post.hpp>

namespace ozo::detail {

template <typename Handler>
struct post_handler {
    Handler handler;

    post_handler(Handler handler) : handler(std::move(handler)) {}

    template <typename Connection>
    void operator() (error_code ec, Connection&& connection) {
        decltype(auto) io = get_io_context(connection);
        asio::post(io, detail::bind(std::move(handler), std::move(ec), std::forward<Connection>(connection)));
    }
};

} // namespace ozo::detail
