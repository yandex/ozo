#pragma once

#include <ozo/asio.h>
#include <ozo/error.h>
#include <ozo/deadline.h>
#include <ozo/detail/bind.h>

#include <boost/asio/dispatch.hpp>

namespace ozo::detail {

template <typename Executor, typename Continuation, typename TimerHandler>
class deadline_handler {
public:
    template <typename TimeConstraint>
    deadline_handler(const Executor& ex, TimeConstraint t, Continuation handler, TimerHandler on_deadline)
    : timer_(ozo::detail::get_operation_timer(ex, t)), executor_(ozo::detail::make_strand_executor(ex)) {
        auto allocator = asio::get_associated_allocator(handler);
        ctx_ = std::allocate_shared<context>(allocator, std::move(handler), std::move(on_deadline));
        timer_.async_wait(wrapper{get_executor(), get_allocator(), ctx_});
    }

    template <typename ...Args>
    void operator() (error_code ec, Args&& ...args) {
        if (--ctx_->state_) {
            timer_.cancel();
            asio::dispatch(ozo::detail::bind(std::move(ctx_->handler_), std::move(ec), std::forward<Args>(args)...));
        } else {
            asio::dispatch(ozo::detail::bind(std::move(ctx_->handler_), asio::error::timed_out, std::forward<Args>(args)...));
        }
    }

    using timer_type = typename ozo::detail::operation_timer<Executor>::type;

    using executor_type = ozo::detail::strand<Executor>;

    executor_type get_executor() const noexcept { return executor_;}

    using allocator_type = asio::associated_allocator_t<Continuation>;

    allocator_type get_allocator() const noexcept { return asio::get_associated_allocator(ctx_->handler_);}

private:

    timer_type timer_;
    executor_type executor_;
    struct context {
        Continuation handler_;
        TimerHandler on_deadline_;
        std::atomic<int> state_{2};

        context(Continuation&& handler, TimerHandler&& on_deadline)
        : handler_(std::move(handler)), on_deadline_(std::move(on_deadline)) {
        }
    };

    std::shared_ptr<context> ctx_;

    struct wrapper {
        using executor_type = deadline_handler::executor_type;

        executor_type get_executor() const noexcept { return executor_;}

        using allocator_type = deadline_handler::allocator_type;

        allocator_type get_allocator() const noexcept { return allocator_;}

        void operator() (error_code ec) {
            if (ec != asio::error::operation_aborted) {
                if (--(ctx_->state_)) {
                    asio::dispatch(ozo::detail::bind(std::move(ctx_->on_deadline_), std::move(ec)));
                }
            }
        }

        executor_type executor_;
        allocator_type allocator_;
        std::shared_ptr<context> ctx_;
    };

};

} // namespace ozo::detail
