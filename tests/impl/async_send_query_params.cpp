#include <connection_mock.h>
#include <test_error.h>

#include <ozo/impl/async_request.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace {

namespace asio = boost::asio;

using namespace testing;
using namespace ozo::tests;

using callback_mock = callback_gmock<connection_ptr<>>;

struct fixture {
    StrictMock<connection_gmock> connection{};
    StrictMock<PGconn_mock> native_handle{};
    StrictMock<callback_mock> callback{};
    io_context io;
    execution_context cb_io;
    connection_ptr<> conn = make_connection(connection, io, native_handle);
    ozo::binary_query query = ozo::to_binary_query(empty_query {}, ozo::empty_oid_map_c);

    auto make_operation_context() {
        EXPECT_CALL(callback, get_executor()).WillRepeatedly(Return(cb_io.get_executor()));
        return ozo::impl::make_request_operation_context(conn, wrap(callback));
    }

    decltype(ozo::impl::make_request_operation_context(conn, wrap(callback))) ctx;

    fixture() : ctx(make_operation_context()) {}

};

using ozo::error_code;

struct async_send_query_params_op : Test {
    fixture m;
};

TEST_F(async_send_query_params_op, should_set_non_blocking_mode_and_send_query_params_and_post_continuation_in_connection_executor) {
    const InSequence s;

    EXPECT_CALL(m.native_handle, PQsetnonblocking(1)).WillOnce(Return(0));
    EXPECT_CALL(m.native_handle, PQsendQueryParams(_, _, _, _, _, _, _)).WillOnce(Return(1));
    EXPECT_CALL(m.native_handle, PQflush())
        .WillOnce(Return(1));
    EXPECT_CALL(m.connection, async_wait_write(_))
        .WillOnce(Return());

    ozo::impl::async_send_query_params_op(m.ctx, m.query).perform();

    EXPECT_EQ(m.ctx->state, ozo::impl::query_state::send_in_progress);
}

TEST_F(async_send_query_params_op, should_set_error_state_and_cancel_io_and_invoke_callback_with_error_if_pg_set_nonbloking_failed) {
    const InSequence s;

    EXPECT_CALL(m.native_handle, PQsetnonblocking(1)).WillOnce(Return(-1));
    EXPECT_CALL(m.connection, cancel()).WillOnce(Return());
    EXPECT_CALL(m.callback, call(error_code{ozo::error::pg_set_nonblocking_failed}, _))
        .WillOnce(Return());

    ozo::impl::async_send_query_params_op(m.ctx, m.query).perform();

    EXPECT_EQ(m.ctx->state, ozo::impl::query_state::error);
}

TEST_F(async_send_query_params_op, should_call_handler_with_error_if_send_query_params_returns_error) {
    Sequence s;
    EXPECT_CALL(m.native_handle, PQsetnonblocking(1)).InSequence(s).WillOnce(Return(0));
    EXPECT_CALL(m.native_handle, PQsendQueryParams(_, _, _, _, _, _, _)).InSequence(s).WillOnce(Return(0));
    EXPECT_CALL(m.connection, cancel()).InSequence(s).WillOnce(Return());
    EXPECT_CALL(m.callback, call(error_code{ozo::error::pg_send_query_params_failed}, _))
        .InSequence(s).WillOnce(Return());

    ozo::impl::async_send_query_params_op(m.ctx, m.query).perform();

    EXPECT_EQ(m.ctx->state, ozo::impl::query_state::error);
}

TEST_F(async_send_query_params_op, should_exit_immediately_if_query_state_is_error_and_called_with_no_error) {
    m.ctx->state = ozo::impl::query_state::error;

    ozo::impl::async_send_query_params_op(m.ctx, m.query)();

    EXPECT_EQ(m.ctx->state, ozo::impl::query_state::error);
}

TEST_F(async_send_query_params_op, should_exit_immediately_if_query_state_is_error_and_called_with_error) {
    m.ctx->state = ozo::impl::query_state::error;

    ozo::impl::async_send_query_params_op(m.ctx, m.query)(error::error);

    EXPECT_EQ(m.ctx->state, ozo::impl::query_state::error);
}

TEST_F(async_send_query_params_op, should_exit_immediately_if_query_state_is_send_finish_and_called_with_no_error) {
    m.ctx->state = ozo::impl::query_state::send_finish;

    ozo::impl::async_send_query_params_op(m.ctx, m.query)();

    EXPECT_EQ(m.ctx->state, ozo::impl::query_state::send_finish);
}

TEST_F(async_send_query_params_op, should_exit_immediately_if_query_state_is_send_finish_and_called_with_error) {
    m.ctx->state = ozo::impl::query_state::send_finish;

    ozo::impl::async_send_query_params_op(m.ctx, m.query)(error::error);

    EXPECT_EQ(m.ctx->state, ozo::impl::query_state::send_finish);
}

TEST_F(async_send_query_params_op, should_invoke_callback_with_given_error_if_called_with_error_and_query_state_is_send_in_progress) {
    const InSequence s;

    EXPECT_CALL(m.connection, cancel()).WillOnce(Return());
    EXPECT_CALL(m.callback, call(error_code{error::error}, _))
        .WillOnce(Return());

    m.ctx->state = ozo::impl::query_state::send_in_progress;
    ozo::impl::async_send_query_params_op(m.ctx, m.query)(error::error);

    EXPECT_EQ(m.ctx->state, ozo::impl::query_state::error);
}

TEST_F(async_send_query_params_op, should_exit_if_flush_output_returns_send_finish) {
    EXPECT_CALL(m.native_handle, PQflush())
        .WillOnce(Return(0));

    m.ctx->state = ozo::impl::query_state::send_in_progress;
    ozo::impl::async_send_query_params_op(m.ctx, m.query)();

    EXPECT_EQ(m.ctx->state, ozo::impl::query_state::send_finish);
}

TEST_F(async_send_query_params_op, should_invoke_callback_with_pg_flush_failed_if_flush_output_returns_error) {
    const InSequence s;

    EXPECT_CALL(m.native_handle, PQflush())
        .WillOnce(Return(-1));
    EXPECT_CALL(m.connection, cancel()).WillOnce(Return());
    EXPECT_CALL(m.callback, call(error_code{ozo::error::pg_flush_failed}, _))
        .WillOnce(Return());

    m.ctx->state = ozo::impl::query_state::send_in_progress;
    ozo::impl::async_send_query_params_op(m.ctx, m.query)();

    EXPECT_EQ(m.ctx->state, ozo::impl::query_state::error);
}

TEST_F(async_send_query_params_op, should_wait_for_write_if_flush_output_returns_send_in_progress) {
    const InSequence s;

    EXPECT_CALL(m.native_handle, PQflush())
        .WillOnce(Return(1));
    EXPECT_CALL(m.connection, async_wait_write(_))
        .WillOnce(Return());

    m.ctx->state = ozo::impl::query_state::send_in_progress;
    ozo::impl::async_send_query_params_op(m.ctx, m.query)();

    EXPECT_EQ(m.ctx->state, ozo::impl::query_state::send_in_progress);
}

TEST_F(async_send_query_params_op, should_wait_for_write_in_strand) {
    Sequence s;
    EXPECT_CALL(m.native_handle, PQflush()).InSequence(s)
        .WillOnce(Return(1));
    EXPECT_CALL(m.connection, async_wait_write(_)).InSequence(s).WillOnce(InvokeArgument<0>(error_code{}));
    EXPECT_CALL(m.cb_io.executor_, post(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.native_handle, PQflush()).InSequence(s)
        .WillOnce(Return(0));

    m.ctx->state = ozo::impl::query_state::send_in_progress;
    ozo::impl::async_send_query_params_op(m.ctx, m.query)();
}

} // namespace
