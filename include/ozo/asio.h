#pragma once

#include <boost/asio/io_service.hpp>
#include <boost/asio/executor.hpp>
#include <boost/asio/strand.hpp>

namespace ozo {

namespace asio = boost::asio;
using asio::async_completion;
using asio::io_context;

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

