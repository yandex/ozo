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

template <typename Stream, typename Handler, typename Result>
class io_deadline_handler {
public:
    template <typename TimeConstraint>
    io_deadline_handler (Stream& stream, const TimeConstraint& t, Handler handler)
    : timer_(ozo::detail::get_operation_timer(stream.get_executor(), t)) {
        auto allocator = asio::get_associated_allocator(handler);
        ctx_ = std::allocate_shared<context>(allocator, stream, std::move(handler));
        timer_.async_wait(timer_handler{ctx_});
    }

    void operator() (error_code ec, Result result) {
        if (--ctx_->first_call) {
            timer_.cancel();
            ctx_->ec = std::move(ec);
            ctx_->result = std::move(result);
            ctx_.reset();
        } else {
            auto handler = std::move(ctx_->handler);
            auto ec = std::move(ctx_->ec);
            ctx_.reset();
            handler(std::move(ec), std::move(result));
        }
    }

    using executor_type = asio::associated_executor_t<Handler>;

    executor_type get_executor() const noexcept { return asio::get_associated_executor(ctx_->handler);}

    using allocator_type = asio::associated_allocator_t<Handler>;

    allocator_type get_allocator() const noexcept { return asio::get_associated_allocator(ctx_->handler);}

private:
    using timer_type = typename ozo::detail::operation_timer<typename Stream::executor_type>::type;

    struct context {
        Stream& stream;
        Handler handler;
        Result result;
        error_code ec;
        std::atomic<long int> first_call{2};

        context(Stream& stream, Handler&& handler)
        : stream(stream), handler(std::move(handler)) {
        }
    };

    timer_type timer_;
    std::shared_ptr<context> ctx_;

    struct timer_handler {
        using executor_type = asio::associated_executor_t<Handler>;

        executor_type get_executor() const noexcept { return asio::get_associated_executor(ctx_->handler);}

        using allocator_type = asio::associated_allocator_t<Handler>;

        allocator_type get_allocator() const noexcept { return asio::get_associated_allocator(ctx_->handler);}

        void operator() (error_code) {
            if (--ctx_->first_call) {
                ctx_->stream.cancel();
                ctx_->ec = asio::error::timed_out;
                ctx_.reset();
            } else {
                auto handler = std::move(ctx_->handler);
                auto ec = std::move(ctx_->ec);
                auto result = std::move(ctx_->result);
                ctx_.reset();
                handler(std::move(ec), std::move(result));
            }
        }

        std::shared_ptr<context> ctx_;
    };
};

} // namespace ozo::detail
