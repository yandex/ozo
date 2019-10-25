#include "test_asio.h"

#include <ozo/detail/make_copyable.h>

#include <boost/asio/post.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace {

using namespace testing;
using namespace ozo::tests;

namespace asio = boost::asio;

struct make_copyable : Test {
    StrictMock<callback_gmock<int>> cb_mock {};
    ozo::tests::execution_context io;
};

struct handler_mock {
    MOCK_METHOD2(call_ref, void(ozo::error_code, int));
    MOCK_METHOD2(call_rval_ref, void(ozo::error_code, int));
    MOCK_CONST_METHOD2(call_const_ref, void(ozo::error_code, int));
};

struct handler_obj {
    handler_mock& mock_;

    void operator()(ozo::error_code ec, int v) & { mock_.call_ref(ec, v); }
    void operator()(ozo::error_code ec, int v) && { mock_.call_rval_ref(ec, v); }
    void operator()(ozo::error_code ec, int v) const & { mock_.call_const_ref(ec, v); }
};

TEST_F(make_copyable, should_provide_handler_executor) {
    EXPECT_CALL(cb_mock, get_executor()).WillOnce(Return(io.get_executor()));

    EXPECT_EQ(ozo::detail::make_copyable(wrap(cb_mock)).get_executor(), io.get_executor());
}

TEST_F(make_copyable, should_call_wrapped_handler) {
    handler_mock mock;
    auto obj = ozo::detail::make_copyable(handler_obj{mock});

    EXPECT_CALL(mock, call_ref(ozo::error_code{}, 42));
    static_cast<decltype(obj)&>(obj)(ozo::error_code{}, 42);

    EXPECT_CALL(mock, call_const_ref(ozo::error_code{}, 42));
    static_cast<const decltype(obj)&>(obj)(ozo::error_code{}, 42);

    EXPECT_CALL(mock, call_rval_ref(ozo::error_code{}, 42));
    static_cast<decltype(obj)&&>(obj)(ozo::error_code{}, 42);
}

TEST(make_copyable_t, should_forward_copyable_handler) {
    struct copyable {};
    EXPECT_TRUE((std::is_same_v<ozo::detail::make_copyable_t<copyable>, copyable>));
}

TEST(make_copyable_t, should_wrap_non_copyable_handler) {
    struct non_copyable {
        std::unique_ptr<int> v_;
    };
    EXPECT_TRUE((std::is_same_v<ozo::detail::make_copyable_t<non_copyable>, ozo::detail::make_copyable<non_copyable>>));
}

} // namespace
