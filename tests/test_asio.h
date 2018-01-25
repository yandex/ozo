#pragma once

#include <ozo/asio.h>

#include <GUnit/GTest.h>

namespace ozo {
namespace testing {

template <typename Handler>
void asio_post(Handler h) {
    using boost::asio::asio_handler_invoke;
    asio_handler_invoke(h, std::addressof(h));
}

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


} // namespace testing
} // namespace ozo
