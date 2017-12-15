#pragma once

#include <boost/asio.hpp>

namespace libapq {

namespace asio = ::boost::asio;

using io_context = asio::io_service;

} // namespace libapq
