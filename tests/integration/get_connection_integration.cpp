#include <ozo/connection_info.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace {

TEST(get_connection, should_return_timeout_error_for_zero_connect_timeout) {
    ozo::io_context io;
    const ozo::connection_info conn_info(OZO_PG_TEST_CONNINFO);
    const std::chrono::seconds timeout(0);

    std::atomic_flag called {};
    ozo::get_connection(conn_info[io], timeout, [&] (ozo::error_code ec, auto conn) {
        EXPECT_FALSE(called.test_and_set());
        EXPECT_EQ(ec, boost::asio::error::timed_out);
        EXPECT_FALSE(ozo::connection_bad(conn));
        EXPECT_EQ(ozo::get_error_context(conn), "error while connection polling");
    });

    io.run();
}

TEST(get_connection, should_return_connection_for_max_connect_timeout) {
    ozo::io_context io;
    const ozo::connection_info conn_info(OZO_PG_TEST_CONNINFO);
    const auto timeout = ozo::time_traits::duration::max();

    std::atomic_flag called {};
    ozo::get_connection(conn_info[io], timeout, [&] (ozo::error_code ec, auto conn) {
        EXPECT_FALSE(called.test_and_set());
        EXPECT_FALSE(ec);
        EXPECT_FALSE(ozo::connection_bad(conn));
    });

    io.run();
}

} // namespace
