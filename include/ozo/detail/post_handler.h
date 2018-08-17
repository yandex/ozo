#pragma once

#include <ozo/asio.h>
#include <ozo/error.h>
#include <ozo/connection.h>

#include <boost/asio/post.hpp>

namespace ozo::detail {

template <typename Handler>
struct post_handler {
    Handler handler;

    template <typename Connection>
    void operator() (error_code ec, Connection&& connection) {
        decltype(auto) io = get_io_context(connection);
        asio::post(io, detail::bind(std::move(handler), std::move(ec), std::forward<Connection>(connection)));
    }
};

template <typename Handler>
inline auto make_post_handler(Handler&& handler) {
    using result_type = post_handler<std::decay_t<Handler>>;
    return result_type {std::forward<Handler>(handler)};
}

} // namespace ozo::detail
