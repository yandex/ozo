#pragma once

#include <ozo/asio.h>

#include <atomic>

namespace ozo::detail {

/**
 * @brief Wrapper for Handler object to be called only once
 *
 * Wrapped handler will be called no more than once per
 * wrapper life-time.
 */
template <typename Handler>
struct call_once {
    call_once(Handler handler) : handler_(std::move(handler)) {}

    template <typename ...Args>
    void operator()(Args&& ...args) {
        if (!called_.test_and_set(std::memory_order_acquire)) {
            handler_(std::forward<Args>(args)...);
        }
    }

    using target_type = std::decay_t<Handler>;

    using executor_type = asio::associated_executor_t<target_type>;

    executor_type get_executor() const noexcept { return asio::get_associated_executor(handler_);}

    using allocator_type = asio::associated_allocator_t<target_type>;

    allocator_type get_allocator() const noexcept { return asio::get_associated_allocator(handler_);}

    call_once(const call_once&) = delete;
    call_once(call_once&&) = delete;
    call_once& operator = (const call_once&) = delete;
    call_once& operator = (call_once&&) = delete;

    target_type handler_;
    std::atomic_flag called_ = ATOMIC_FLAG_INIT;
};

} // namespace ozo::detail
