#pragma once

#include "test_error.h"

#include <ozo/detail/bind.h>
#include <ozo/asio.h>

#include <boost/asio/post.hpp>
#include <boost/asio/executor.hpp>
#include <boost/asio/bind_executor.hpp>

#include <gmock/gmock.h>

namespace ozo {
namespace tests {

struct executor_mock {
    virtual ~executor_mock() = default;
    virtual void dispatch(std::function<void ()>) const = 0;
    virtual void post(std::function<void ()>) const = 0;
    virtual void defer(std::function<void ()>) const = 0;
};

struct executor_gmock : executor_mock {
    MOCK_CONST_METHOD1(dispatch, void (std::function<void ()>));
    MOCK_CONST_METHOD1(post, void (std::function<void ()>));
    MOCK_CONST_METHOD1(defer, void (std::function<void ()>));
};

template <typename Handler>
struct shared_wrapper {
    std::shared_ptr<Handler> ptr;

    template <typename ... Args>
    void operator ()(Args&& ... args) {
        return (*ptr)(std::forward<Args>(args) ...);
    }
};

template <typename Function>
auto wrap_shared(Function&& f) {
    return shared_wrapper<std::decay_t<Function>> {std::make_shared<std::decay_t<Function>>(std::forward<Function>(f))};
}

struct strand_executor_service_mock {
    virtual ~strand_executor_service_mock() = default;
    virtual executor_mock& get_executor() = 0;
};

struct strand_executor_service_gmock : strand_executor_service_mock {
    MOCK_METHOD0(get_executor, executor_mock& ());
};

struct execution_context : asio::execution_context {
    struct executor_type {
        executor_mock* impl_ = nullptr;
        execution_context* context_ = nullptr;

        executor_type(executor_mock& impl, execution_context& context)
        : impl_(&impl), context_(&context){}

        execution_context& context() const noexcept {
            return *context_;
        }

        void on_work_started() const {}

        void on_work_finished() const {}

        template <typename Function>
        void dispatch(Function&& f, std::allocator<void>) const {
            return impl_->dispatch(wrap_shared(std::forward<Function>(f)));
        }

        template <typename Function>
        void post(Function&& f, std::allocator<void>) const {
            return impl_->post(wrap_shared(std::forward<Function>(f)));
        }

        template <typename Function>
        void defer(Function&& f, std::allocator<void>) const {
            return impl_->defer(wrap_shared(std::forward<Function>(f)));
        }

        friend bool operator ==(const executor_type& lhs, const executor_type& rhs) {
            return lhs.context_ == rhs.context_ && lhs.impl_ == rhs.impl_;
        }
    };

    executor_mock* executor_ = nullptr;
    strand_executor_service_mock* strand_service_ = nullptr;

    execution_context() = default;

    execution_context(executor_mock& executor)
        : executor_ {&executor} {}

    execution_context(executor_mock& executor, strand_executor_service_mock& strand_service)
        : executor_ {&executor}, strand_service_(&strand_service) {}

    executor_type get_executor() {
        if (!executor_) {
            throw std::logic_error("ozo::testing::execution_context::get_executor() bad executor mock");
        }
        return {*executor_, *this};
    }
};

using io_context = execution_context;
using executor = execution_context::executor_type;
using strand = executor;

struct stream_descriptor_mock {
    virtual void async_write_some(std::function<void(error_code)> handler) = 0;
    virtual void async_read_some(std::function<void(error_code)> handler) = 0;
    virtual void cancel(error_code&) = 0;
    virtual void close(error_code&) = 0;
    virtual ~stream_descriptor_mock() = default;
};

struct stream_descriptor_gmock : stream_descriptor_mock {
    MOCK_METHOD1(async_write_some, void(std::function<void(error_code)>));
    MOCK_METHOD1(async_read_some, void(std::function<void(error_code)>));
    MOCK_METHOD1(cancel, void(error_code&));
    MOCK_METHOD1(close, void(error_code&));
};

struct stream_descriptor {
    io_context* io_ = nullptr;
    stream_descriptor_mock* mock_ = nullptr;

    stream_descriptor() = default;
    stream_descriptor(io_context& io, stream_descriptor_mock& mock)
    : io_(&io), mock_(&mock) {}

    template <typename ConstBufferSequence, typename WriteHandler>
    void async_write_some(ConstBufferSequence const &, WriteHandler&& h) {
        mock_->async_write_some([h = std::forward<WriteHandler>(h)] (auto e) {
            asio::post(ozo::detail::bind(std::move(h), std::move(e)));
        });
    }

    template <typename BufferSequence, typename ReadHandler>
    void async_read_some(BufferSequence&&, ReadHandler&& h) {
        mock_->async_read_some([h = std::forward<ReadHandler>(h)] (auto e) {
            asio::post(ozo::detail::bind(std::move(h), std::move(e)));
        });
    }

    void cancel(error_code& ec) { mock_->cancel(ec);}

    void close(error_code& ec) {
        mock_->close(ec);
    }

    io_context& get_io_context() { return *io_;}

    auto get_executor() {
        return get_io_context().get_executor();
    }
};

struct steady_timer_mock {
    virtual ~steady_timer_mock() = default;
    virtual std::size_t expires_after(const asio::steady_timer::duration& expiry_time) = 0;
    virtual std::size_t expires_at(const asio::steady_timer::time_point&) = 0;
    virtual void async_wait(std::function<void(error_code)> handler) = 0;
    virtual std::size_t cancel() = 0;
};

struct steady_timer_gmock : steady_timer_mock {
    MOCK_METHOD1(expires_after, std::size_t (const asio::steady_timer::duration&));
    MOCK_METHOD1(expires_at, std::size_t (const asio::steady_timer::time_point&));
    MOCK_METHOD1(async_wait, void (std::function<void(error_code)>));
    MOCK_METHOD0(cancel, std::size_t ());
};

struct steady_timer {
    steady_timer_mock* impl = nullptr;

    std::size_t expires_after(const asio::steady_timer::duration& expiry_time) {
        return impl->expires_after(expiry_time);
    }

    std::size_t expires_at(const asio::steady_timer::time_point& at) {
        return impl->expires_at(at);
    }

    template <typename Handler>
    void async_wait(Handler&& handler) {
        return impl->async_wait([h = std::forward<Handler>(handler)] (auto e) {
            asio::post(ozo::detail::bind(std::move(h), std::move(e)));
        });
    }

    std::size_t cancel() {
        return impl->cancel();
    }
};

} // namespace tests

namespace detail {

template <>
struct strand_executor<ozo::tests::executor> {
    using type = ozo::tests::executor;

    static auto get(const tests::executor& ex) {
        return type{ex.context().strand_service_->get_executor(), ex.context()};
    }
};

} // namespace detail

namespace tests {

template <typename ... Args>
struct callback_mock {
    virtual void call(ozo::error_code, Args...) const = 0;
    virtual ~callback_mock() = default;
};

template <typename ...Args>
struct callback_gmock;

template <typename Arg1, typename Arg2>
struct callback_gmock<Arg1, Arg2> {
    using executor_type = boost::asio::executor;

    MOCK_CONST_METHOD3_T(call, void(ozo::error_code, Arg1, Arg2));
    MOCK_CONST_METHOD0_T(get_executor, executor_type ());
};

template <typename Arg>
struct callback_gmock<Arg> {
    using executor_type = boost::asio::executor;

    MOCK_CONST_METHOD2_T(call, void(ozo::error_code, Arg));
    MOCK_CONST_METHOD0_T(get_executor, executor_type ());
};

template <>
struct callback_gmock<> {
    using executor_type = boost::asio::executor;

    MOCK_CONST_METHOD1_T(call, void(ozo::error_code));
    MOCK_CONST_METHOD0_T(get_executor, executor_type ());
};

template <typename M>
struct callback_handler {
    using executor_type = typename M::executor_type;

    M* mock_ = nullptr;

    callback_handler(M& mock) : mock_(std::addressof(mock)) {}

    template <typename ...Args>
    void operator ()(ozo::error_code ec, Args&&... args) const {
        mock_->call(ec, std::forward<Args>(args)...);
    }

    auto get_executor() const noexcept {
        return mock_->get_executor();
    }
};

template <typename T>
inline callback_handler<typename T::type> wrap(T& mock) {
    return {object(mock)};
}

template <typename ...Ts>
inline callback_handler<callback_gmock<Ts...>> wrap(callback_gmock<Ts...>& mock) {
    return {mock};
}

template <typename ...Ts>
inline callback_handler<callback_gmock<Ts...>> wrap(testing::StrictMock<callback_gmock<Ts...>>& mock) {
    return {mock};
}

template <typename T, typename Executor>
inline auto wrap(T& mock, const Executor& e) {
    return asio::bind_executor(e, callback_handler<typename T::type>{object(mock)});
}

template <typename Executor, typename ...Ts>
inline auto wrap(callback_gmock<Ts...>& mock, const Executor& e) {
    return asio::bind_executor(e, callback_handler<callback_gmock<Ts...>>{mock});
}

template <typename Executor, typename ...Ts>
inline auto wrap(testing::StrictMock<callback_gmock<Ts...>>& mock, const Executor& e) {
    return asio::bind_executor(e, callback_handler<callback_gmock<Ts...>>{mock});
}

} // namespace tests
} // namespace ozo
