#pragma once

#include <boost/asio/io_service.hpp>

namespace ozo {

namespace asio = boost::asio;
using asio::async_completion;
using io_context = asio::io_service;

template <typename ExecutionContext>
struct asio_strand { using type = typename ExecutionContext::strand; };

template <typename ExecutionContext>
using strand = typename asio_strand<std::decay_t<ExecutionContext>>::type;

template <typename ExecutionContext>
auto make_strand_executor(ExecutionContext& ctx) {
    return strand<ExecutionContext>{ctx};
}

} // namespace ozo
