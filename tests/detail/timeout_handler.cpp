#include <ozo/detail/timeout_handler.h>
#include "../test_asio.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace {

using namespace testing;

struct connection_mock {
    MOCK_METHOD0(cancel, void());
};

TEST(cancel_io, should_cancel_socket_io) {
    connection_mock conn;
    ozo::detail::cancel_io cancel_io_handler{conn, std::allocator<char>{}};
    EXPECT_CALL(conn, cancel()).WillOnce(Return());
    cancel_io_handler(ozo::error_code{});
}

} // namespace
