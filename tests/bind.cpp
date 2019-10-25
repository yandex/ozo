#include "test_asio.h"

#include <ozo/detail/bind.h>

#include <boost/asio/post.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace {

using namespace testing;
using namespace ozo::tests;

namespace asio = boost::asio;

struct bind : Test {
    StrictMock<callback_gmock<int>> cb_mock {};
    ozo::tests::execution_context io;
};

TEST_F(bind, should_use_handler_executor) {
    const InSequence s;

    EXPECT_CALL(cb_mock, get_executor()).WillOnce(Return(io.get_executor()));
    EXPECT_CALL(io.executor_, post(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(cb_mock, call(_, _)).WillOnce(Return());

    asio::post(ozo::detail::bind(wrap(cb_mock), ozo::error_code{}, 42));
}

TEST_F(bind, should_forward_binded_values) {
    EXPECT_CALL(cb_mock, call(ozo::error_code{}, 42)).WillOnce(Return());

    ozo::detail::bind(wrap(cb_mock), ozo::error_code{}, 42)();
}

} // namespace
