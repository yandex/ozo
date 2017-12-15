#include <apq/detail/bind.h>
#include <GUnit/GTest.h>

template <typename Handler>
inline void asio_post(Handler h) {
    using boost::asio::asio_handler_invoke;
    asio_handler_invoke(h, std::addressof(h));
}

struct callback_mock {
    virtual void call(int) const = 0;
    virtual void context_preserved() const = 0;
    virtual ~callback_mock() = default;
};

struct callback_handler {
    callback_mock& mock;

    void operator() (int arg) const {
        mock.call(arg);
    }

    template <typename Func>
    friend void asio_handler_invoke(Func&& f, callback_handler* ctx) {
        ctx->mock.context_preserved();
        f();
    }
};

inline auto wrap(callback_mock& mock) {
    return callback_handler{mock};
}

GTEST("libapq::detail::bind()") {
    SHOULD("preserve handler context") {
        using namespace ::testing;
        StrictGMock<callback_mock> cb_mock{};

        EXPECT_INVOKE(cb_mock, context_preserved);
        EXPECT_INVOKE(cb_mock, call, _);

        asio_post(libapq::detail::bind(wrap(object(cb_mock)), 42));
    }

    SHOULD("forward binded value") {
        using namespace ::testing;
        StrictGMock<callback_mock> cb_mock{};

        EXPECT_INVOKE(cb_mock, context_preserved);
        EXPECT_INVOKE(cb_mock, call, 42);

        asio_post(libapq::detail::bind(wrap(object(cb_mock)), 42));
    }
}
