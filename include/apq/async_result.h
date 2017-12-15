#pragma once

#include <boost/asio/async_result.hpp>
#include <utility>
#include <apq/asio.h>

namespace libapq {

//We can reuse boost::asio::async_result since it is not binded to error code type
using asio::async_result;
using asio::handler_type;

/**
* This is a part of compatibility layer with Boos.Asio 1.66
*/
template <typename CompletionToken, typename Signature>
struct async_completion {
    explicit async_completion(CompletionToken& token)
    : completion_handler(std::move(token)),
      result(completion_handler) {
    }

    using completion_handler_type = typename libapq::handler_type<CompletionToken, Signature>::type;

    completion_handler_type completion_handler;
    async_result<completion_handler_type> result;
};

} // namespace libapq
