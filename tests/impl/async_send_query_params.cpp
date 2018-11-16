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
    StrictMock<executor_gmock> callback_executor{};
    StrictMock<callback_mock> callback{};
    StrictMock<executor_gmock> executor{};
    StrictMock<executor_gmock> strand{};
    StrictMock<strand_executor_service_gmock> strand_service{};
    StrictMock<stream_descriptor_gmock> socket{};
    StrictMock<steady_timer_gmock> timer{};
    io_context io{executor, strand_service};
    decltype(make_connection(connection, io, socket, timer)) conn =
            make_connection(connection, io, socket, timer);

    auto make_operation_context() {
        EXPECT_CALL(strand_service, get_executor()).WillOnce(ReturnRef(strand));
        return ozo::impl::make_request_operation_context(conn, wrap(callback));
    }

    decltype(ozo::impl::make_request_operation_context(conn, wrap(callback))) ctx;

    fixture() : ctx(make_operation_context()) {}

};

using ozo::error_code;

struct async_send_query_params_op : Test {
    fixture m;
};

TEST_F(async_send_query_params_op, should_set_non_blocking_mode_and_send_query_params_and_post_continuation_in_strand) {
    const InSequence s;

    EXPECT_CALL(m.connection, set_nonblocking()).WillOnce(Return(0));
    EXPECT_CALL(m.connection, send_query_params()).WillOnce(Return(1));
    EXPECT_CALL(m.executor, post(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.strand, dispatch(_)).WillOnce(Return());

    ozo::impl::make_async_send_query_params_op(m.ctx, fake_query{}).perform();

    EXPECT_EQ(m.ctx->state, ozo::impl::query_state::flushing);
}

TEST_F(async_send_query_params_op, should_set_error_state_and_cancel_io_and_invoke_callback_with_error_if_pg_set_nonbloking_failed) {
    const InSequence s;

    EXPECT_CALL(m.connection, set_nonblocking()).WillOnce(Return(-1));
    EXPECT_CALL(m.socket, cancel(_)).WillOnce(Return());
    EXPECT_CALL(m.callback, get_executor()).WillOnce(Return(ozo::tests::executor {&m.callback_executor}));
    EXPECT_CALL(m.strand, post(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.callback_executor, dispatch(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.callback, call(error_code{ozo::error::pg_set_nonblocking_failed}, _))
        .WillOnce(Return());

    ozo::impl::make_async_send_query_params_op(m.ctx, fake_query{}).perform();

    EXPECT_EQ(m.ctx->state, ozo::impl::query_state::error);
}

TEST_F(async_send_query_params_op, should_call_send_query_params_while_it_returns_error) {
    // According to the documentation
    //   In the nonblocking state, calls to PQsendQuery, PQputline,
    //   PQputnbytes, PQputCopyData, and PQendcopy will not block
    //   but instead return an error if they need to be called again.
    // PQsendQueryParams is PQsendQuery family function so it must
    // conform to the same rules.

    Sequence s;
    EXPECT_CALL(m.connection, set_nonblocking()).InSequence(s).WillOnce(Return(0));
    EXPECT_CALL(m.connection, send_query_params()).InSequence(s).WillOnce(Return(0));
    EXPECT_CALL(m.connection, send_query_params()).InSequence(s).WillOnce(Return(0));
    EXPECT_CALL(m.connection, send_query_params()).InSequence(s).WillOnce(Return(1));
    EXPECT_CALL(m.executor, post(_)).InSequence(s).WillOnce(Return());

    ozo::impl::make_async_send_query_params_op(m.ctx, fake_query{}).perform();

    EXPECT_EQ(m.ctx->state, ozo::impl::query_state::flushing);
}

TEST_F(async_send_query_params_op, should_exit_immediately_if_query_state_is_error_and_called_with_no_error) {
    m.ctx->state = ozo::impl::query_state::error;

    ozo::impl::make_async_send_query_params_op(m.ctx, fake_query{})();

    EXPECT_EQ(m.ctx->state, ozo::impl::query_state::error);
}

TEST_F(async_send_query_params_op, should_exit_immediately_if_query_state_is_error_and_called_with_error) {
    m.ctx->state = ozo::impl::query_state::error;

    ozo::impl::make_async_send_query_params_op(m.ctx, fake_query{})(error::error);

    EXPECT_EQ(m.ctx->state, ozo::impl::query_state::error);
}

TEST_F(async_send_query_params_op, should_exit_immediately_if_query_state_is_done_and_called_with_no_error) {
    m.ctx->state = ozo::impl::query_state::done;

    ozo::impl::make_async_send_query_params_op(m.ctx, fake_query{})();

    EXPECT_EQ(m.ctx->state, ozo::impl::query_state::done);
}

TEST_F(async_send_query_params_op, should_exit_immediately_if_query_state_is_done_and_called_with_error) {
    m.ctx->state = ozo::impl::query_state::done;

    ozo::impl::make_async_send_query_params_op(m.ctx, fake_query{})(error::error);

    EXPECT_EQ(m.ctx->state, ozo::impl::query_state::done);
}

TEST_F(async_send_query_params_op, should_invoke_callback_with_given_error_if_called_with_error_and_query_state_is_flushing) {
    const InSequence s;

    EXPECT_CALL(m.socket, cancel(_)).WillOnce(Return());
    EXPECT_CALL(m.callback, get_executor()).WillOnce(Return(ozo::tests::executor {&m.callback_executor}));
    EXPECT_CALL(m.strand, post(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.callback_executor, dispatch(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.callback, call(error_code{error::error}, _))
        .WillOnce(Return());

    m.ctx->state = ozo::impl::query_state::flushing;
    ozo::impl::make_async_send_query_params_op(m.ctx, fake_query{})(error::error);

    EXPECT_EQ(m.ctx->state, ozo::impl::query_state::error);
}

TEST_F(async_send_query_params_op, should_exit_if_flush_output_returns_done) {
    EXPECT_CALL(m.connection, flush_output())
        .WillOnce(Return(ozo::impl::flush_result::success));

    m.ctx->state = ozo::impl::query_state::flushing;
    ozo::impl::make_async_send_query_params_op(m.ctx, fake_query{})();

    EXPECT_EQ(m.ctx->state, ozo::impl::query_state::done);
}

TEST_F(async_send_query_params_op, should_invoke_callback_with_pg_flush_failed_if_flush_output_returns_error) {
    const InSequence s;

    EXPECT_CALL(m.connection, flush_output())
        .WillOnce(Return(ozo::impl::flush_result::error));
    EXPECT_CALL(m.socket, cancel(_)).WillOnce(Return());
    EXPECT_CALL(m.callback, get_executor()).WillOnce(Return(ozo::tests::executor {&m.callback_executor}));
    EXPECT_CALL(m.strand, post(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.callback_executor, dispatch(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.callback, call(error_code{ozo::error::pg_flush_failed}, _))
        .WillOnce(Return());

    m.ctx->state = ozo::impl::query_state::flushing;
    ozo::impl::make_async_send_query_params_op(m.ctx, fake_query{})();

    EXPECT_EQ(m.ctx->state, ozo::impl::query_state::error);
}

TEST_F(async_send_query_params_op, should_wait_for_write_if_flush_output_returns_flushing) {
    const InSequence s;

    EXPECT_CALL(m.connection, flush_output())
        .WillOnce(Return(ozo::impl::flush_result::send_in_progress));
    EXPECT_CALL(m.socket, async_write_some(_))
        .WillOnce(Return());

    m.ctx->state = ozo::impl::query_state::flushing;
    ozo::impl::make_async_send_query_params_op(m.ctx, fake_query{})();

    EXPECT_EQ(m.ctx->state, ozo::impl::query_state::flushing);
}

TEST_F(async_send_query_params_op, should_wait_for_write_in_strand) {
    Sequence s;
    EXPECT_CALL(m.connection, flush_output()).InSequence(s)
        .WillOnce(Return(ozo::impl::flush_result::send_in_progress));
    EXPECT_CALL(m.socket, async_write_some(_)).InSequence(s).WillOnce(InvokeArgument<0>(error_code{}));
    EXPECT_CALL(m.strand, post(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.connection, flush_output()).InSequence(s)
        .WillOnce(Return(ozo::impl::flush_result::success));

    m.ctx->state = ozo::impl::query_state::flushing;
    ozo::impl::make_async_send_query_params_op(m.ctx, fake_query{})();
}

} // namespace
