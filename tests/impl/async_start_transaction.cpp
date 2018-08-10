#include "connection_mock.h"

#include <ozo/impl/async_start_transaction.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace {

using namespace testing;
using namespace ozo::tests;

using ozo::error_code;

struct async_start_transaction : Test {
    StrictMock<connection_gmock> connection {};
    StrictMock<callback_gmock<ozo::impl::transaction<connection_ptr<>>>> callback {};
    StrictMock<executor_gmock> executor {};
    StrictMock<executor_gmock> strand {};
    StrictMock<strand_executor_service_gmock> strand_service {};
    StrictMock<stream_descriptor_gmock> socket {};
    StrictMock<steady_timer_gmock> timer {};
    io_context io {executor, strand_service};
    decltype(make_connection(connection, io, socket, timer)) conn = make_connection(connection, io, socket, timer);
};

TEST_F(async_start_transaction, should_call_async_execute) {
    *conn->handle_ = native_handle::good;

    Sequence s;

    EXPECT_CALL(executor, dispatch(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(callback, context_preserved()).InSequence(s).WillOnce(Return());
    EXPECT_CALL(strand_service, get_executor()).InSequence(s).WillOnce(ReturnRef(strand));

    // Send query params
    EXPECT_CALL(connection, set_nonblocking()).InSequence(s).WillOnce(Return(0));
    EXPECT_CALL(connection, send_query_params()).InSequence(s).WillOnce(Return(1));

    EXPECT_CALL(executor, post(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(strand, dispatch(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(callback, context_preserved()).InSequence(s).WillOnce(Return());
    EXPECT_CALL(connection, flush_output()).InSequence(s).WillOnce(Return(ozo::impl::query_state::send_finish));

    // Get result
    EXPECT_CALL(executor, post(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(strand, dispatch(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(callback, context_preserved()).InSequence(s).WillOnce(Return());
    EXPECT_CALL(connection, is_busy()).InSequence(s).WillOnce(Return(false));
    EXPECT_CALL(connection, get_result()).InSequence(s).WillOnce(Return(boost::none));

    EXPECT_CALL(executor, post(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(strand, dispatch(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(callback, context_preserved()).InSequence(s).WillOnce(Return());

    EXPECT_CALL(executor, post(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(callback, context_preserved()).InSequence(s).WillOnce(Return());
    EXPECT_CALL(callback, call(error_code {}, _)).InSequence(s).WillOnce(Return());
    EXPECT_CALL(socket, close(_)).InSequence(s).WillOnce(Return());

    ozo::impl::async_start_transaction(conn, fake_query {}, wrap(callback));
}

TEST_F(async_start_transaction, should_close_connection_when_destruct_last_transaction_copy_with_connection) {
    ozo::impl::transaction<decltype(conn)> transaction;

    *conn->handle_ = native_handle::good;

    Sequence s;

    EXPECT_CALL(executor, dispatch(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(callback, context_preserved()).InSequence(s).WillOnce(Return());
    EXPECT_CALL(strand_service, get_executor()).InSequence(s).WillOnce(ReturnRef(strand));

    // Send query params
    EXPECT_CALL(connection, set_nonblocking()).InSequence(s).WillOnce(Return(0));
    EXPECT_CALL(connection, send_query_params()).InSequence(s).WillOnce(Return(1));

    EXPECT_CALL(executor, post(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(strand, dispatch(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(callback, context_preserved()).InSequence(s).WillOnce(Return());
    EXPECT_CALL(connection, flush_output()).InSequence(s).WillOnce(Return(ozo::impl::query_state::send_finish));

    // Get result
    EXPECT_CALL(executor, post(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(strand, dispatch(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(callback, context_preserved()).InSequence(s).WillOnce(Return());
    EXPECT_CALL(connection, is_busy()).InSequence(s).WillOnce(Return(false));
    EXPECT_CALL(connection, get_result()).InSequence(s).WillOnce(Return(boost::none));

    EXPECT_CALL(executor, post(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(strand, dispatch(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(callback, context_preserved()).InSequence(s).WillOnce(Return());

    EXPECT_CALL(executor, post(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(callback, context_preserved()).InSequence(s).WillOnce(Return());
    EXPECT_CALL(callback, call(error_code {}, _)).InSequence(s).WillOnce(SaveArg<1>(&transaction));

    ozo::impl::async_start_transaction(conn, fake_query {}, wrap(callback));

    EXPECT_CALL(socket, close(_)).InSequence(s).WillOnce(Return());
}

} // namespace
