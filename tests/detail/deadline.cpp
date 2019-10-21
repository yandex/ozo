#include <ozo/detail/deadline.h>
#include "../test_asio.h"
#include "../test_error.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

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
    ozo::tests::steady_timer_service_mock timer_service;
    ozo::tests::execution_context io{executor, strand_service, timer_service};
    StrictMock<ozo::tests::callback_gmock<>> on_deadline;
    StrictMock<ozo::tests::callback_gmock<>> continuation;

    deadline_handler() {
        EXPECT_CALL(timer_service, timer(An<time_point>())).WillRepeatedly(ReturnRef(timer));
        EXPECT_CALL(strand_service, get_executor()).WillOnce(ReturnRef(strand));
    }
};

TEST_F(deadline_handler, should_call_timeout_handler_on_timeout) {
    ozo::tests::executor_gmock on_deadline_executor;
    EXPECT_CALL(timer, async_wait(_)).WillOnce(InvokeArgument<0>(ozo::error_code{}));
    EXPECT_CALL(strand, post(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(on_deadline, get_executor())
        .WillOnce(Return(ozo::tests::executor{on_deadline_executor, io}));
    EXPECT_CALL(on_deadline_executor, dispatch(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(on_deadline, call(_));
    using ozo::tests::wrap;
    ozo::detail::deadline_handler(io.get_executor(), time_point{}, wrap(continuation), wrap(on_deadline));
}

TEST_F(deadline_handler, should_not_call_timeout_handler_on_timer_cancel) {
    EXPECT_CALL(timer, async_wait(_)).WillOnce(InvokeArgument<0>(boost::asio::error::operation_aborted));
    EXPECT_CALL(strand, post(_)).WillOnce(InvokeArgument<0>());
    using ozo::tests::wrap;
    ozo::detail::deadline_handler(io.get_executor(), time_point{}, wrap(continuation), wrap(on_deadline));
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
    auto h = ozo::detail::deadline_handler(io.get_executor(), time_point{}, wrap(continuation), wrap(on_deadline));
    h(ozo::error_code{});
}

struct stream_mock {
    using executor_type = ozo::tests::io_context::executor_type;
    MOCK_METHOD0(cancel, void());
    MOCK_METHOD0(get_executor, executor_type());
};

struct io_deadline_handler : Test {
    ozo::tests::executor_gmock executor;
    ozo::tests::strand_executor_service_gmock strand_service;
    ozo::tests::steady_timer_gmock timer;
    ozo::tests::steady_timer_service_mock timer_service;
    ozo::tests::execution_context io{executor, strand_service, timer_service};
    StrictMock<ozo::tests::callback_gmock<int>> continuation;
    StrictMock<ozo::tests::executor_gmock> continuation_executor;
    StrictMock<stream_mock> stream;
    std::function<void (ozo::error_code)> on_timer_expired;

    io_deadline_handler() {
        EXPECT_CALL(timer_service, timer(An<time_point>())).WillRepeatedly(ReturnRef(timer));
        EXPECT_CALL(stream, get_executor()).WillRepeatedly(Return(io.get_executor()));
        EXPECT_CALL(continuation, get_executor())
            .WillRepeatedly(Return(ozo::tests::executor{continuation_executor, io}));
        EXPECT_CALL(timer, async_wait(_)).WillOnce(SaveArg<0>(&on_timer_expired));
    }

    using deadline_handler = ozo::detail::io_deadline_handler<stream_mock, std::decay_t<decltype(ozo::tests::wrap(continuation))>, int>;
};

TEST_F(io_deadline_handler, should_cancel_stream_io_and_call_handler_with_timeout_error_and_result_on_timer_expired) {
    InSequence s;
    EXPECT_CALL(continuation_executor, post(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(stream, cancel());
    EXPECT_CALL(continuation, call(Eq(boost::asio::error::timed_out), 42));
    using ozo::tests::wrap;
    deadline_handler handler{stream, time_point{}, wrap(continuation)};
    on_timer_expired(ozo::error_code{});
    handler(boost::asio::error::operation_aborted, 42);
}

TEST_F(io_deadline_handler, should_cancel_timer_and_call_handler_with_error_and_result_on_normal_call) {
    InSequence s;
    EXPECT_CALL(timer, cancel());
    EXPECT_CALL(continuation_executor, post(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(continuation, call(Eq(ozo::tests::error::error), 777));
    using ozo::tests::wrap;
    deadline_handler handler{stream, time_point{}, wrap(continuation)};
    handler(ozo::tests::error::error, 777);
    on_timer_expired(boost::asio::error::operation_aborted);
}

} // namespace
