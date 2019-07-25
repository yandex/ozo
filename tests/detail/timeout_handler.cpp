#include <ozo/detail/timeout_handler.h>
#include "../test_asio.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace {

using namespace testing;

TEST(cancel_socket, should_cancel_socket_io) {
    ozo::tests::stream_descriptor_gmock socket;
    ozo::detail::cancel_socket cancel_socket_handler{socket, std::allocator<char>{}};
    EXPECT_CALL(socket, cancel(_)).WillOnce(Return());
    cancel_socket_handler(ozo::error_code{});
}

} // namespace
