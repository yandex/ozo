#include "test_asio.h"

#include <ozo/detail/call_once.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace {

using namespace testing;

TEST(call_once, should_call_callback_only_once) {
    StrictMock<ozo::tests::callback_gmock<>> callback {};
    auto wrapper = ozo::detail::call_once(ozo::tests::wrap(callback));
    EXPECT_CALL(callback, call(_));
    wrapper(ozo::error_code{});
    wrapper(ozo::error_code{});
}

TEST(call_once, should_call_callback_with_arguments) {
    StrictMock<ozo::tests::callback_gmock<int>> callback {};
    auto wrapper = ozo::detail::call_once(ozo::tests::wrap(callback));
    EXPECT_CALL(callback, call(ozo::error_code{}, 42));
    wrapper(ozo::error_code{}, 42);
}

} // namespace