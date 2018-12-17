#pragma once

#include <ozo/asio.h>
#include <ozo/error.h>
#include <ozo/deadline.h>
#include <ozo/detail/bind.h>

#include <boost/asio/dispatch.hpp>

namespace ozo::detail {

template <typename IoContext, typename Continuation>
class deadline_handler {
public:
    template <typename TimeConstraint, typename TimerHandler>
    deadline_handler(IoContext& io, TimeConstraint t, Continuation handler, TimerHandler on_deadline)
    : timer_(ozo::detail::get_operation_timer(io, t)), handler_(std::move(handler)),
            executor_(ozo::detail::make_strand_executor(io.get_executor())) {
        timer_.async_wait(wrapper<TimerHandler>{std::move(on_deadline), get_executor()});
    }

    template <typename ...Args>
    void operator() (error_code ec, Args&& ...args) {
        timer_.cancel();
        asio::dispatch(ozo::detail::bind(std::move(handler_), std::move(ec), std::forward<Args>(args)...));
    }

    using executor_type = ozo::detail::strand<typename IoContext::executor_type>;

    executor_type get_executor() const noexcept { return executor_;}

    using allocator_type = asio::associated_allocator_t<Continuation>;

    allocator_type get_allocator() const noexcept { return asio::get_associated_allocator(handler_);}

    using timer_type = typename ozo::detail::operation_timer<IoContext>::type;

private:
    template <typename Target>
    struct wrapper {
        using executor_type = deadline_handler::executor_type;

        executor_type get_executor() const noexcept { return executor_;}

        using allocator_type = asio::associated_allocator_t<Target>;

        allocator_type get_allocator() const noexcept { return asio::get_associated_allocator(target_);}

        void operator() (error_code ec) {
            if (ec != asio::error::operation_aborted) {
                asio::dispatch(ozo::detail::bind(std::move(target_), std::move(ec)));
            }
        }

        Target target_;
        executor_type executor_;
    };

    timer_type timer_;
    Continuation handler_;
    executor_type executor_;
};

} // namespace ozo::detail
