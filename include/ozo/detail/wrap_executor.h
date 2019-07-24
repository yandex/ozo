#pragma once

#include <ozo/asio.h>

#include <boost/asio/dispatch.hpp>

namespace ozo::detail {
/**
 * @brief Safely wraps handler with a given Executor
 *
 * Comparing to asio::bind_executor this object dispatches handler to its
 * associated executor.
 */
template <typename Executor, typename Handler>
struct wrap_executor {
    Executor ex;
    Handler handler;

    wrap_executor(const Executor& ex, Handler handler)
    : ex(ex), handler(std::move(handler)) {}

    template <typename ...Args>
    void operator() (Args&& ...args) {
        asio::dispatch(detail::bind(std::move(handler), std::forward<Args>(args)...));
    }

    using executor_type = Executor;

    executor_type get_executor() const noexcept { return ex;}

    using allocator_type = asio::associated_allocator_t<Handler>;

    allocator_type get_allocator() const noexcept { return asio::get_associated_allocator(handler);}
};

} // namespace ozo::detail
