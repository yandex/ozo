#include <ozo/connection.h>
#include <ozo/connection_info.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace {

TEST(get_connection, should_return_error_and_bad_connect_for_invalid_connection_info) {
    ozo::io_context io;
    ozo::connection_info<> conn_info("invalid connection info");

    ozo::result res;
    ozo::get_connection(conn_info, io, [](ozo::error_code ec, auto conn) {
        EXPECT_TRUE(ec);
        EXPECT_TRUE(ozo::connection_bad(conn));
        EXPECT_EQ(ozo::error_message(conn), "missing \"=\" after \"invalid\" in connection info string");
    });

    io.run();
}

} // namespace
