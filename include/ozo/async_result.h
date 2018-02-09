#pragma once

#include <boost/asio/async_result.hpp>
#include <utility>
#include <ozo/asio.h>

namespace ozo {

// We can reuse boost::asio::async_result since it is not binded to error code type
using asio::async_result;
using asio::handler_type;

#if (BOOST_ASIO_VERSION >= 101200)

using asio::async_completion;

#else

/**
* This is a part of compatibility layer with Boos.Asio 1.66
*/
template <typename CompletionToken, typename Signature>
struct async_completion {
    explicit async_completion(CompletionToken& token)
    : completion_handler(std::move(token)),
      result(completion_handler) {
    }

    using completion_handler_type = typename ozo::handler_type<CompletionToken, Signature>::type;

    completion_handler_type completion_handler;
    async_result<completion_handler_type> result;
};

#endif

} // namespace ozo
