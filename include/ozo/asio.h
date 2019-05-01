#pragma once

#include <boost/asio/io_service.hpp>
#include <boost/asio/executor.hpp>
#include <boost/asio/strand.hpp>

namespace ozo {

namespace asio = boost::asio;
using asio::async_completion;
using asio::io_context;

#if BOOST_VERSION < 107000

template <typename CompletionToken, typename Signature,
    typename Initiation, typename... Args>
inline decltype(auto) async_initiate(Initiation&& initiation,
    CompletionToken& token, Args&&... args) {
  async_completion<CompletionToken, Signature> completion(token);

  initiation(std::move(completion.completion_handler), std::forward<Args>(args)...);

  return completion.result.get();
}

#else

using asio::async_initiate;

#endif

namespace detail {

template <typename Executor>
struct strand_executor {
    using type = asio::strand<Executor>;

    static auto get(const Executor& ex = Executor{}) {
        return type{ex};
    }
};

template <typename Executor>
using strand = typename strand_executor<std::decay_t<Executor>>::type;

template <typename Executor>
auto make_strand_executor(const Executor& ex) {
    return strand_executor<Executor>::get(ex);
}

} // namespace detail

} // namespace ozo
