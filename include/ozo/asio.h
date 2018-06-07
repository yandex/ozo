#pragma once

#include <boost/asio.hpp>

namespace ozo {

namespace asio = ::boost::asio;
using asio::async_completion;
using io_context = asio::io_service;

template <typename Executor>
struct asio_strand { using type = typename Executor::strand; };

template <typename Executor>
using strand = typename asio_strand<std::decay_t<Executor>>::type;

template <typename Oper>
inline decltype(auto) bind_executor(asio::io_service::strand& s, Oper&& op) {
    return s.wrap(std::forward<Oper>(op));
}

} // namespace ozo
