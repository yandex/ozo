#include <connection_mock.h>
#include <test_error.h>

#include <ozo/impl/async_request.h>
#include <ozo/time_traits.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace {

namespace hana = boost::hana;

using namespace testing;
using namespace ozo::tests;

using callback_mock = callback_gmock<connection_ptr<>>;

using ozo::impl::query_state;
using ozo::error_code;
using ozo::time_traits;

struct async_request_op : Test {
    StrictMock<connection_gmock> connection {};
    StrictMock<executor_gmock> callback_executor {};
    StrictMock<callback_mock> callback {};
    StrictMock<executor_gmock> executor {};
    StrictMock<executor_gmock> strand {};
    StrictMock<strand_executor_service_gmock> strand_service {};
    StrictMock<stream_descriptor_gmock> socket {};
    StrictMock<steady_timer_gmock> timer {};
    StrictMock<steady_timer_service_mock> timer_service;
    io_context io {executor, strand_service, timer_service};
    execution_context cb_io {callback_executor};
    decltype(make_connection(connection, io, socket)) conn =
            make_connection(connection, io, socket);
    time_traits::duration timeout {42};
};

TEST_F(async_request_op, should_set_timer_and_send_query_params_and_get_result_and_call_handler) {

    EXPECT_CALL(strand_service, get_executor()).WillOnce(ReturnRef(strand));
    EXPECT_CALL(callback, get_executor()).WillRepeatedly(Return(cb_io.get_executor()));
    EXPECT_CALL(timer_service, timer(time_traits::duration(42))).WillRepeatedly(ReturnRef(timer));

    Sequence s;

    // Set timer
    EXPECT_CALL(timer, async_wait(_)).InSequence(s).WillOnce(Return());

    // Send query params
    EXPECT_CALL(connection, set_nonblocking()).InSequence(s).WillOnce(Return(0));
    EXPECT_CALL(connection, send_query_params()).InSequence(s).WillOnce(Return(1));

    EXPECT_CALL(connection, flush_output()).InSequence(s).WillOnce(Return(ozo::impl::query_state::send_finish));

    // Get result
    EXPECT_CALL(connection, is_busy()).InSequence(s).WillOnce(Return(false));
    EXPECT_CALL(connection, get_result()).InSequence(s).WillOnce(Return(boost::none));

    // Cancel timer
    EXPECT_CALL(timer, cancel()).InSequence(s).WillOnce(Return(1));

    // Call client handler
    EXPECT_CALL(callback_executor, dispatch(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(callback, call(error_code {}, _)).InSequence(s).WillOnce(Return());

    ozo::impl::async_request_op{fake_query {}, timeout, [] (auto, auto) {}, wrap(callback)}(error_code {}, conn);
}

TEST_F(async_request_op, should_send_query_params_and_get_result_and_call_handler_whith_no_time_constraint) {

    EXPECT_CALL(strand_service, get_executor()).WillOnce(ReturnRef(strand));
    EXPECT_CALL(callback, get_executor()).WillRepeatedly(Return(cb_io.get_executor()));

    Sequence s;

    // Send query params
    EXPECT_CALL(connection, set_nonblocking()).InSequence(s).WillOnce(Return(0));
    EXPECT_CALL(connection, send_query_params()).InSequence(s).WillOnce(Return(1));

    EXPECT_CALL(connection, flush_output()).InSequence(s).WillOnce(Return(ozo::impl::query_state::send_finish));

    // Get result
    EXPECT_CALL(connection, is_busy()).InSequence(s).WillOnce(Return(false));
    EXPECT_CALL(connection, get_result()).InSequence(s).WillOnce(Return(boost::none));

    // Call client handler
    EXPECT_CALL(callback_executor, dispatch(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(callback, call(error_code {}, _)).InSequence(s).WillOnce(Return());

    ozo::impl::async_request_op{fake_query {}, ozo::none, [] (auto, auto) {}, wrap(callback)}(error_code {}, conn);
}

TEST_F(async_request_op, should_cancel_socket_on_timeout) {
    Sequence s;

    EXPECT_CALL(strand_service, get_executor()).InSequence(s).WillOnce(ReturnRef(strand));

    // Set timer
    EXPECT_CALL(timer_service, timer(time_traits::duration(42))).WillRepeatedly(ReturnRef(timer));
    EXPECT_CALL(timer, async_wait(_)).InSequence(s).WillOnce(InvokeArgument<0>(error_code {}));

    EXPECT_CALL(strand, post(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(socket, cancel(_)).InSequence(s).WillOnce(Return());

    // Send query params
    EXPECT_CALL(connection, set_nonblocking()).InSequence(s).WillOnce(Return(0));
    EXPECT_CALL(connection, send_query_params()).InSequence(s).WillOnce(Return(1));
    EXPECT_CALL(connection, flush_output()).InSequence(s).WillOnce(Return(ozo::impl::query_state::send_finish));


    EXPECT_CALL(connection, is_busy()).InSequence(s).WillOnce(Return(true));
    EXPECT_CALL(socket, async_read_some(_)).InSequence(s).WillOnce(InvokeArgument<0>(error_code {}));
    EXPECT_CALL(strand, post(_)).InSequence(s).WillOnce(Return());

    ozo::impl::async_request_op{fake_query {}, timeout, [] (auto, auto) {}, wrap(callback)}(error_code {}, conn);
}

} // namespace
