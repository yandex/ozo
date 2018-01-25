#pragma once

#include <boost/asio.hpp>

namespace ozo {

namespace asio = ::boost::asio;

using io_context = asio::io_service;

} // namespace ozo
