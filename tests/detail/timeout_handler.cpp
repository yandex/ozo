#include <ozo/detail/timeout_handler.h>
#include "../test_asio.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace {

using namespace testing;

struct connection_mock {
    MOCK_METHOD0(cancel, void());
};

TEST(cancel_socket, should_cancel_socket_io) {
    connection_mock conn;
    ozo::detail::cancel_socket cancel_socket_handler{conn, std::allocator<char>{}};
    EXPECT_CALL(conn, cancel()).WillOnce(Return());
    cancel_socket_handler(ozo::error_code{});
}

} // namespace
