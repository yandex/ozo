#include <ozo/core/none.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace {

using namespace testing;

TEST(none_t, should_be_equal_to_none_t_object) {
    EXPECT_EQ(ozo::none_t{}, ozo::none_t{});
}

TEST(none_t, should_be_not_equal_to_any_other_type_object) {
    EXPECT_NE(ozo::none, int{});
    EXPECT_NE(ozo::none, double{});
    EXPECT_NE(ozo::none, std::string{});
}

TEST(none_t, should_return_void_when_called) {
    EXPECT_TRUE(std::is_void_v<decltype(ozo::none(1, 2, "str"))>);
}

TEST(none_t, should_return_void_when_applied) {
    EXPECT_TRUE(std::is_void_v<decltype(ozo::none_t::apply(1, 2, "str"))>);
}

TEST(IsNone, should_return_true_for_none_t) {
    EXPECT_TRUE(ozo::IsNone<ozo::none_t>);
}

TEST(IsNone, should_return_false_for_different_type) {
    EXPECT_FALSE(ozo::IsNone<int>);
    EXPECT_FALSE(ozo::IsNone<class other>);
}

} // namespace
