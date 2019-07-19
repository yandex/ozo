#include <ozo/detail/deadline.h>
#include "../test_asio.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace {

struct io_context_mock : ozo::tests::execution_context {
    ozo::tests::steady_timer_gmock* timer_;
    io_context_mock(ozo::tests::executor_mock& executor,
        ozo::tests::strand_executor_service_mock& strand_service,
        ozo::tests::steady_timer_gmock& timer
    ) : ozo::tests::execution_context(executor, strand_service), timer_(std::addressof(timer)) {}
    MOCK_METHOD1(get_operation_timer, void(ozo::time_traits::time_point));
};

} // namespace

namespace ozo::detail {

template <>
struct operation_timer<io_context_mock> {
    using type = ozo::tests::steady_timer;

    template <typename TimeConstraint>
    static type get(io_context_mock& io, TimeConstraint t) {
        io.get_operation_timer(t);
        return type{io.timer_};
    }
};

} // namespace ozo::detail

namespace {

using namespace testing;
using namespace std::literals;

using time_point = ozo::time_traits::time_point;
using duration = ozo::time_traits::duration;

struct deadline_handler : Test {
    ozo::tests::executor_gmock executor;
    ozo::tests::executor_gmock strand;
    ozo::tests::strand_executor_service_gmock strand_service;
    ozo::tests::steady_timer_gmock timer;
    io_context_mock io{executor, strand_service, timer};
    StrictMock<ozo::tests::callback_gmock<>> on_deadline;
    StrictMock<ozo::tests::callback_gmock<>> continuation;

    deadline_handler() {
        EXPECT_CALL(io, get_operation_timer(_));
        EXPECT_CALL(strand_service, get_executor()).WillOnce(ReturnRef(strand));
    }
};

TEST_F(deadline_handler, should_call_timeout_handler_on_timeout) {
    EXPECT_CALL(timer, async_wait(_)).WillOnce(InvokeArgument<0>(ozo::error_code{}));
    EXPECT_CALL(strand, post(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(on_deadline, call(_));
    using ozo::tests::wrap;
    ozo::detail::deadline_handler(io, ozo::time_traits::time_point{}, wrap(continuation), wrap(on_deadline));
}

TEST_F(deadline_handler, should_not_call_timeout_handler_on_timer_cancel) {
    EXPECT_CALL(timer, async_wait(_)).WillOnce(InvokeArgument<0>(boost::asio::error::operation_aborted));
    EXPECT_CALL(strand, post(_)).WillOnce(InvokeArgument<0>());
    using ozo::tests::wrap;
    ozo::detail::deadline_handler(io, ozo::time_traits::time_point{}, wrap(continuation), wrap(on_deadline));
}

TEST_F(deadline_handler, should_cancel_timer_and_call_continuation) {
    ozo::tests::executor_gmock continuation_executor;
    EXPECT_CALL(timer, async_wait(_));
    EXPECT_CALL(timer, cancel());
    EXPECT_CALL(continuation, get_executor())
        .WillOnce(Return(ozo::tests::executor{continuation_executor, io}));
    EXPECT_CALL(continuation_executor, dispatch(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(continuation, call(_));
    using ozo::tests::wrap;
    auto h = ozo::detail::deadline_handler(io, ozo::time_traits::time_point{}, wrap(continuation), wrap(on_deadline));
    h(ozo::error_code{});
}

} // namespace
