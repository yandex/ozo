#include "test_asio.h"

#include <ozo/detail/bind.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace {

using namespace testing;
using namespace ozo::tests;

struct bind : Test {
    StrictMock<callback_gmock<int>> cb_mock{};
};

TEST_F(bind, should_preserve_handler_context) {
    EXPECT_CALL(cb_mock, context_preserved()).WillOnce(Return());
    EXPECT_CALL(cb_mock, call(_, _)).WillOnce(Return());

    asio_post(ozo::detail::bind(wrap(cb_mock), ozo::error_code{}, 42));
}

TEST_F(bind, should_forward_binded_values) {
    EXPECT_CALL(cb_mock, context_preserved()).WillOnce(Return());
    EXPECT_CALL(cb_mock, call(ozo::error_code{}, 42)).WillOnce(Return());

    asio_post(ozo::detail::bind(wrap(cb_mock), ozo::error_code{}, 42));
}

} // namespace
