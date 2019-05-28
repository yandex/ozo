#include <ozo/detail/timeout_handler.h>
#include "../test_asio.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace {

using namespace testing;
using namespace std::literals;

using time_point = ozo::time_traits::time_point;
using duration = ozo::time_traits::duration;

struct test_handler {
    template <typename ...Ts>
    void operator() (Ts&&...) const {}
};

struct connection {
    ozo::tests::steady_timer_gmock timer;
    ozo::tests::stream_descriptor_gmock socket;
    friend auto& get_timer(connection& c) { return c.timer; }
    friend auto& get_socket(connection& c) { return c.socket; }
};

TEST(set_io_timeout, should_do_nothing_for_ozo_none) {
    connection c;
    ozo::detail::set_io_timeout(c, test_handler{}, ozo::none);
}

TEST(set_io_timeout, should_set_timer_for_duration_and_async_wait) {
    connection c;
    EXPECT_CALL(c.timer, expires_after(duration(1s))).WillOnce(Return(0));
    EXPECT_CALL(c.timer, async_wait(_)).WillOnce(Return());
    ozo::detail::set_io_timeout(c, test_handler{}, duration(1s));
}

TEST(set_io_timeout, should_set_timer_for_time_point_and_async_wait) {
    connection c;
    EXPECT_CALL(c.timer, expires_at(time_point{})).WillOnce(Return(0));
    EXPECT_CALL(c.timer, async_wait(_)).WillOnce(Return());
    ozo::detail::set_io_timeout(c, test_handler{}, time_point{});
}

} // namespace
