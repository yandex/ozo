#pragma once

#include <ozo/asio.h>

#include <GUnit/GTest.h>
#include <boost/core/demangle.hpp>

namespace ozo {
namespace testing {

template <typename Handler>
inline void asio_post(Handler h) {
    using boost::asio::asio_handler_invoke;
    asio_handler_invoke(h, std::addressof(h));
}

struct executor_mock {
    virtual void post(std::function<void()>) const = 0;
    virtual void dispatch(std::function<void()>) const = 0;
    virtual ~executor_mock() = default;
};

template <typename Op>
inline void post(executor_mock& e, Op op) {
    e.post([op=std::move(op)] () mutable { asio_post(std::move(op));});
}

template <typename Op>
inline void dispatch(executor_mock& e, Op op) {
    e.dispatch([op=std::move(op)] () mutable { asio_post(std::move(op));});
}

struct strand_executor_service_mock {
    virtual executor_mock& get_executor() const = 0;
    virtual ~strand_executor_service_mock() = default;
};

struct io_context {
    executor_mock& executor_;
    strand_executor_service_mock& strand_;

    template <typename Handler>
    void post(Handler&& h) {
        ::ozo::testing::post(executor_, std::forward<Handler>(h));
    }

    template <typename Handler>
    void dispatch(Handler&& h) {
        ::ozo::testing::dispatch(executor_, std::forward<Handler>(h));
    }
};

struct strand {
    executor_mock& e_;

    strand(const io_context& io) : e_(io.strand_.get_executor()) {}

    template <typename Handler>
    void post(Handler&& h) { ::ozo::testing::post(e_, std::forward<Handler>(h));}

    template <typename Handler>
    void dispatch(Handler&& h) { ::ozo::testing::dispatch(e_, std::forward<Handler>(h));}

    template <typename Op>
    struct wrapper {
        executor_mock& e_;
        std::decay_t<Op> op_;
        std::shared_ptr<bool> active_;

        template <typename ...Args>
        decltype(auto) operator() (Args&& ...args) {
            return op_(std::forward<Args>(args)...);
        }

        template <typename Func>
        friend void asio_handler_invoke(Func&& f, wrapper* ctx) {
            if (!*(ctx->active_)) {
                auto active = ctx->active_;
                *active = true;
                ::ozo::testing::dispatch(ctx->e_, std::forward<Func>(f));
                *active = false;
            } else {
                using boost::asio::asio_handler_invoke;
                asio_handler_invoke(f, std::addressof(ctx->op_));
            }
        }
    };

    template <typename Op>
    decltype(auto) wrap(Op&& op) {
        return wrapper<Op>{e_, std::forward<Op>(op), std::make_shared<bool>(false)};
    }

    template <typename Op>
    friend decltype(auto) bind_executor(strand& s, Op&& op) {
        return s.wrap(std::forward<Op>(op));
    }
};

struct stream_descriptor_mock {
    virtual void async_write_some(std::function<void(error_code)> handler) = 0;
    virtual void async_read_some(std::function<void(error_code)> handler) = 0;
    virtual void cancel(error_code&) = 0;
    virtual ~stream_descriptor_mock() = default;
};

struct stream_descriptor {
    io_context* io_ = nullptr;
    stream_descriptor_mock* mock_ = nullptr;

    stream_descriptor() = default;
    stream_descriptor(io_context& io, stream_descriptor_mock& mock)
    : io_(&io), mock_(&mock) {}

    template <typename ConstBufferSequence, typename WriteHandler>
    void async_write_some(ConstBufferSequence const &, WriteHandler&& h) {
        mock_->async_write_some([h=std::forward<WriteHandler>(h)] (auto e) {
            asio_post(ozo::detail::bind(std::move(h), std::move(e)));
        });
    }

    template <typename BufferSequence, typename ReadHandler>
    void async_read_some(BufferSequence&&, ReadHandler&& h) {
        mock_->async_read_some([h=std::forward<ReadHandler>(h)] (auto e) {
            asio_post(ozo::detail::bind(std::move(h), std::move(e)));
        });
    }

    void cancel(error_code& ec) { mock_->cancel(ec);}

    io_context& get_io_service() { return *io_;}
};

} // namespace testing

template <>
struct asio_strand<testing::io_context> { using type = testing::strand; };

namespace testing {

struct socket_mock {
    socket_mock& get_io_service() { return *this;}
    template <typename Handler>
    void post(Handler&& h) {
        asio_post(std::forward<Handler>(h));
    }
    template <typename Handler>
    void dispatch(Handler&& h) {
        asio_post(std::forward<Handler>(h));
    }
};

template <typename ... Args>
struct callback_mock {
    virtual void call(ozo::error_code, Args...) const = 0;
    virtual void context_preserved() const = 0;
    virtual ~callback_mock() = default;
};

template <typename Arg>
struct callback_gmock {
    MOCK_CONST_METHOD2_T(call, void(ozo::error_code, Arg));
    MOCK_CONST_METHOD0_T(context_preserved, void());
};

template <typename M>
struct callback_handler{
    M& mock_;

    template <typename ...Args>
    void operator() (ozo::error_code ec, Args&&... args) const {
        mock_.call(ec, std::forward<Args>(args)...);
    }

    template <typename Func>
    friend void asio_handler_invoke(Func&& f, callback_handler* ctx) {
        ctx->mock_.context_preserved();
        f();
    }
};

template <typename T>
inline callback_handler<typename T::type> wrap(T& mock) {
    return {object(mock)};
}

template <typename T>
inline callback_handler<callback_gmock<T>> wrap(callback_gmock<T>& mock) {
    return {mock};
}

template <typename T>
inline callback_handler<callback_gmock<T>> wrap(::testing::StrictMock<callback_gmock<T>>& mock) {
    return {mock};
}

} // namespace testing
} // namespace ozo
