#include <ozo/detail/cancel_timer_handler.h>

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

TEST(bind_cancel_timer, should_forward_handler_for_ozo_none_t) {
    EXPECT_TRUE((std::is_same_v<
        decltype(ozo::detail::bind_cancel_timer<ozo::none_t>(test_handler{})),
        test_handler&&>));
}

TEST(bind_cancel_timer, should_wrap_handler_for_ozo_time_traits_duration) {
    EXPECT_TRUE((std::is_same_v<
        decltype(ozo::detail::bind_cancel_timer<ozo::time_traits::duration>(test_handler{})),
        ozo::detail::cancel_timer_handler<test_handler>>));
}

TEST(bind_cancel_timer, should_wrap_handler_for_ozo_time_traits_time_point) {
    EXPECT_TRUE((std::is_same_v<
        decltype(ozo::detail::bind_cancel_timer<ozo::time_traits::time_point>(test_handler{})),
        ozo::detail::cancel_timer_handler<test_handler>>));
}

} // namespace
