#include <ozo/deadline.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace {

using namespace testing;
using namespace std::literals;

using time_point = ozo::time_traits::time_point;
using duration = ozo::time_traits::duration;

TEST(deadline, should_return_its_argument_for_time_point_type) {
    EXPECT_EQ(ozo::deadline(time_point{}), time_point{});
}

TEST(deadline, should_return_none_for_ozo_none_t_type) {
    EXPECT_EQ(ozo::deadline(ozo::none_t{}), ozo::none);
}

TEST(deadline, should_return_proper_time_point_for_time_point_and_duration) {
    EXPECT_EQ(ozo::deadline(1s, time_point{}), time_point{} + 1s);
}

TEST(deadline, should_return_time_point_max_on_saturation) {
    EXPECT_EQ(ozo::deadline(duration::max(), time_point{} + 1s),
        time_point::max());
}

TEST(deadline, should_return_time_point_argument_on_negative_duration) {
    EXPECT_EQ(ozo::deadline(-1s, time_point{}), time_point{});
}

TEST(time_left, should_return_duration_for_time_point_less_than_deadline) {
    EXPECT_EQ(ozo::time_left(time_point{} + 1s, time_point{}), 1s);
}

TEST(time_left, should_return_zero_for_time_point_equal_to_deadline) {
    EXPECT_EQ(ozo::time_left(time_point{}, time_point{}), duration(0));
}

TEST(time_left, should_return_zero_for_time_point_greater_than_deadline) {
    EXPECT_EQ(ozo::time_left(time_point{}, time_point{} + 1s), duration(0));
}

TEST(expired, should_return_false_for_time_point_less_than_deadline) {
    EXPECT_FALSE(ozo::expired(time_point{} + 1s, time_point{}));
}

TEST(expired, should_return_true_for_time_point_equal_to_deadline) {
    EXPECT_TRUE(ozo::expired(time_point{}, time_point{}));
}

TEST(expired, should_return_true_for_time_point_greater_than_deadline) {
    EXPECT_TRUE(ozo::expired(time_point{}, time_point{} + 1s));
}

} // namespace
