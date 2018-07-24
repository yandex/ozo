#include "connection_mock.h"
#include "test_error.h"

#include <ozo/impl/async_request.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace {

namespace hana = boost::hana;

using namespace testing;
using namespace ozo::tests;

using callback_mock = callback_gmock<connection_ptr<>>;

struct fixture {
    StrictMock<connection_gmock> connection{};
    StrictMock<callback_mock> callback{};
    StrictMock<executor_gmock> executor{};
    StrictMock<executor_gmock> strand{};
    StrictMock<strand_executor_service_gmock> strand_service{};
    StrictMock<stream_descriptor_gmock> socket{};
    io_context io{executor, strand_service};
    decltype(make_connection(connection, io, socket)) conn =
            make_connection(connection, io, socket);

    auto make_operation_context() {
        EXPECT_CALL(strand_service, get_executor()).WillOnce(ReturnRef(strand));
        return ozo::impl::make_operation_context(conn, wrap(callback));
    }

    decltype(ozo::impl::make_operation_context(conn, wrap(callback))) ctx;

    fixture() : ctx(make_operation_context()) {}

};

using ozo::impl::query_state;
using ozo::error_code;

struct async_get_result_op : Test {
    fixture m;
};

TEST_F(async_get_result_op, perform_sould_post_continuation_within_strand) {
    EXPECT_CALL(m.executor, post(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.strand, dispatch(_)).WillOnce(Return());

    ozo::impl::make_async_get_result_op(m.ctx, [](auto, auto){}).perform();
}

TEST_F(async_get_result_op, perform_sould_preserve_query_state) {
    EXPECT_CALL(m.executor, post(_)).WillOnce(Return());

    ozo::impl::make_async_get_result_op(m.ctx, [](auto, auto){}).perform();

    EXPECT_EQ(m.ctx->state, query_state::send_in_progress);
}

struct async_get_result_op_call : async_get_result_op,
        WithParamInterface<error_code> {
};

TEST_P(async_get_result_op_call, when_query_state_is_error_should_exit_and_preserve_state){
    m.ctx->state = ozo::impl::query_state::error;
    ozo::impl::make_async_get_result_op(m.ctx, [](auto, auto){})(GetParam());
    EXPECT_EQ(m.ctx->state, query_state::error);
}

INSTANTIATE_TEST_CASE_P(
    with_any_error_code,
    async_get_result_op_call,
    Values(error_code{}, error_code{error::error}));


struct async_get_result_op_call_with_error : async_get_result_op,
        WithParamInterface<query_state> {
};

TEST_P(async_get_result_op_call_with_error, should_post_callback_with_given_error) {
    m.ctx->state = GetParam();

    EXPECT_CALL(m.socket, cancel(_)).WillOnce(Return());
    EXPECT_CALL(m.executor, post(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.strand, dispatch(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.callback, context_preserved()).WillOnce(Return());
    EXPECT_CALL(m.callback, call(error_code{error::error}, _)).WillOnce(Return());

    ozo::impl::make_async_get_result_op(m.ctx, [](auto, auto){})(error::error);
}

TEST_P(async_get_result_op_call_with_error, should_post_callback_with_operation_aborted_if_called_with_bad_descriptor) {
    m.ctx->state = GetParam();

    EXPECT_CALL(m.socket, cancel(_)).WillOnce(Return());
    EXPECT_CALL(m.executor, post(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.strand, dispatch(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.callback, context_preserved()).WillOnce(Return());
    EXPECT_CALL(m.callback, call(error_code{boost::asio::error::operation_aborted}, _)).WillOnce(Return());

    ozo::impl::make_async_get_result_op(m.ctx, [](auto, auto){})(boost::asio::error::bad_descriptor);
}

TEST_P(async_get_result_op_call_with_error, should_set_query_state_in_error) {
    m.ctx->state = GetParam();

    EXPECT_CALL(m.socket, cancel(_)).WillOnce(Return());
    EXPECT_CALL(m.executor, post(_)).WillOnce(Return());

    ozo::impl::make_async_get_result_op(m.ctx, [](auto, auto){})(error::error);

    EXPECT_EQ(m.ctx->state, query_state::error);
}

INSTANTIATE_TEST_CASE_P(
    with_query_state_NOT_error,
    async_get_result_op_call_with_error,
    Values(query_state::send_in_progress, query_state::send_finish));

struct process_mock {
    MOCK_CONST_METHOD0(call, void());
};

struct process_wrapper {
    process_mock& mock;
    template <typename ...Ts>
    void operator() (Ts&& ...) const {mock.call();}
};
struct async_get_result : Test {
    fixture m;
    StrictMock<process_mock> process;
    process_wrapper process_f{process};
};

TEST_F(async_get_result, should_wait_for_read_and_consume_input_while_is_busy_returns_true) {
    Sequence s;
    // Post self to strand
    EXPECT_CALL(m.executor, post(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.strand, dispatch(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.callback, context_preserved()).InSequence(s).WillOnce(Return());

    // wait for read while is_busy() returns true
    EXPECT_CALL(m.connection, is_busy()).InSequence(s).WillOnce(Return(true));
    EXPECT_CALL(m.socket, async_read_some(_)).InSequence(s).WillOnce(InvokeArgument<0>(error_code{}));
    EXPECT_CALL(m.strand, dispatch(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.callback, context_preserved()).InSequence(s).WillOnce(Return());

    // consume input
    EXPECT_CALL(m.connection, consume_input()).InSequence(s).WillOnce(Return(1));
    // wait for read while is_busy() which returns true
    EXPECT_CALL(m.connection, is_busy()).InSequence(s).WillOnce(Return(true));
    EXPECT_CALL(m.socket, async_read_some(_)).InSequence(s).WillOnce(Return());

    ozo::impl::async_get_result(m.ctx, process_f);
}

TEST_F(async_get_result, should_post_callback_with_error_if_consume_input_failed) {
    Sequence s;

    // Post self to strand
    EXPECT_CALL(m.executor, post(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.strand, dispatch(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.callback, context_preserved()).InSequence(s).WillOnce(Return());

    // Wait for read while is_busy() returns true
    EXPECT_CALL(m.connection, is_busy()).InSequence(s).WillOnce(Return(true));
    EXPECT_CALL(m.socket, async_read_some(_)).InSequence(s).WillOnce(InvokeArgument<0>(error_code{}));
    EXPECT_CALL(m.strand, dispatch(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.callback, context_preserved()).InSequence(s).WillOnce(Return());

    // Consume input
    EXPECT_CALL(m.connection, consume_input()).InSequence(s).WillOnce(Return(0));

    // Cancel all io
    EXPECT_CALL(m.socket, cancel(_)).WillOnce(Return());

    // Post callback with error
    EXPECT_CALL(m.executor, post(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.strand, dispatch(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.callback, context_preserved()).InSequence(s).WillOnce(Return());
    EXPECT_CALL(m.callback, call(error_code{ozo::error::pg_consume_input_failed}, _))
        .InSequence(s).WillOnce(Return());

    ozo::impl::async_get_result(m.ctx, process_f);
}

TEST_F(async_get_result, should_process_data_and_post_callback_if_result_is_empty) {
    Sequence s;

    // Post self to strand
    EXPECT_CALL(m.executor, post(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.strand, dispatch(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.callback, context_preserved()).InSequence(s).WillOnce(Return());

    // get result since is_busy() is false
    EXPECT_CALL(m.connection, is_busy()).InSequence(s).WillOnce(Return(false));
    EXPECT_CALL(m.connection, get_result())
        .InSequence(s)
        .WillOnce(Return(boost::none));

    // Post callback with no error since result is empty
    EXPECT_CALL(m.executor, post(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.strand, dispatch(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.callback, context_preserved()).InSequence(s).WillOnce(Return());
    EXPECT_CALL(m.callback, call(error_code{}, _)).InSequence(s).WillOnce(Return());

    ozo::impl::async_get_result(m.ctx, process_f);
}

TEST_F(async_get_result, should_post_callback_with_error_and_consume_if_process_data_throws) {
    Sequence s;

    // Post self to strand
    EXPECT_CALL(m.executor, post(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.strand, dispatch(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.callback, context_preserved()).InSequence(s).WillOnce(Return());

    // get result since is_busy() is false
    EXPECT_CALL(m.connection, is_busy()).InSequence(s).WillOnce(Return(false));
    EXPECT_CALL(m.connection, get_result())
        .InSequence(s)
        .WillOnce(Return(make_pg_result(PGRES_TUPLES_OK, error_code{})));

    EXPECT_CALL(process, call()).InSequence(s).WillOnce(Invoke([]{ throw std::runtime_error("");}));

    EXPECT_CALL(m.socket, cancel(_)).InSequence(s).WillOnce(Return());
    // Post callback with no error since result is error
    EXPECT_CALL(m.executor, post(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.strand, dispatch(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.callback, context_preserved()).InSequence(s).WillOnce(Return());
    EXPECT_CALL(m.callback, call(error_code{ozo::error::bad_result_process}, _))
        .InSequence(s).WillOnce(Return());

    //Consume result with calling get_result until it returns nothing
    EXPECT_CALL(m.connection, get_result())
        .InSequence(s)
        .WillOnce(Return(boost::none));

    ozo::impl::async_get_result(m.ctx, process_f);
}

TEST_F(async_get_result, should_process_data_and_post_callback_and_consume_if_result_status_is_PGRES_TUPLES_OK) {
    Sequence s;

    // Post self to strand
    EXPECT_CALL(m.executor, post(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.strand, dispatch(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.callback, context_preserved()).InSequence(s).WillOnce(Return());

    // get result since is_busy() is false
    EXPECT_CALL(m.connection, is_busy()).InSequence(s).WillOnce(Return(false));
    EXPECT_CALL(m.connection, get_result())
        .InSequence(s)
        .WillOnce(Return(make_pg_result(PGRES_TUPLES_OK, error_code{})));

    EXPECT_CALL(process, call()).InSequence(s).WillOnce(Return());
    // Post callback with no error since result is ok
    EXPECT_CALL(m.executor, post(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.strand, dispatch(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.callback, context_preserved()).InSequence(s).WillOnce(Return());
    EXPECT_CALL(m.callback, call(error_code{}, _)).InSequence(s).WillOnce(Return());

    //Consume result with calling get_result until it returns nothing
    EXPECT_CALL(m.connection, get_result())
        .InSequence(s)
        .WillOnce(Return(boost::none));

    ozo::impl::async_get_result(m.ctx, process_f);
}

TEST_F(async_get_result, should_process_data_and_post_callback_if_result_status_is_PGRES_SINGLE_TUPLE) {
    Sequence s;

    // Post self to strand
    EXPECT_CALL(m.executor, post(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.strand, dispatch(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.callback, context_preserved()).InSequence(s).WillOnce(Return());

    // Get result since is_busy() is false
    EXPECT_CALL(m.connection, is_busy()).InSequence(s).WillOnce(Return(false));
    EXPECT_CALL(m.connection, get_result())
        .InSequence(s)
        .WillOnce(Return(make_pg_result(PGRES_SINGLE_TUPLE, error_code{})));

    // Process result
    EXPECT_CALL(process, call()).InSequence(s).WillOnce(Return());

    // Post callback with no error since result is ok
    EXPECT_CALL(m.executor, post(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.strand, dispatch(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.callback, context_preserved()).InSequence(s).WillOnce(Return());
    EXPECT_CALL(m.callback, call(error_code{}, _)).InSequence(s).WillOnce(Return());

    ozo::impl::async_get_result(m.ctx, process_f);
}

TEST_F(async_get_result, should_post_callback_and_consume_result_if_result_status_is_PGRES_COMMAND_OK) {
    Sequence s;
    // Post self to strand
    EXPECT_CALL(m.executor, post(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.strand, dispatch(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.callback, context_preserved()).InSequence(s).WillOnce(Return());

    // Get result since is_busy() is false
    EXPECT_CALL(m.connection, is_busy()).InSequence(s).WillOnce(Return(false));
    EXPECT_CALL(m.connection, get_result())
        .InSequence(s)
        .WillOnce(Return(make_pg_result(PGRES_COMMAND_OK, error_code{})));

    // Post callback with no error since result is ok
    EXPECT_CALL(m.executor, post(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.strand, dispatch(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.callback, context_preserved()).InSequence(s).WillOnce(Return());
    EXPECT_CALL(m.callback, call(error_code{}, _)).InSequence(s).WillOnce(Return());

    // Consume result with calling get_result until it returns nothing
    EXPECT_CALL(m.connection, get_result())
        .InSequence(s)
        .WillOnce(Return(boost::none));

    ozo::impl::async_get_result(m.ctx, process_f);
}

TEST_F(async_get_result, should_post_callback_with_error_and_consume_result_if_result_status_is_PGRES_BAD_RESPONSE) {
    Sequence s;
    // Post self to strand
    EXPECT_CALL(m.executor, post(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.strand, dispatch(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.callback, context_preserved()).InSequence(s).WillOnce(Return());

    // get result since is_busy() is false
    EXPECT_CALL(m.connection, is_busy()).InSequence(s).WillOnce(Return(false));
    EXPECT_CALL(m.connection, get_result())
        .InSequence(s)
        .WillOnce(Return(make_pg_result(PGRES_BAD_RESPONSE, error_code{})));

    // Post callback with error and cancel all io
    EXPECT_CALL(m.socket, cancel(_)).WillOnce(Return());
    EXPECT_CALL(m.executor, post(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.strand, dispatch(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.callback, context_preserved()).InSequence(s).WillOnce(Return());
    EXPECT_CALL(m.callback, call(error_code{ozo::error::result_status_bad_response}, _))
        .InSequence(s)
        .WillOnce(Return());

    // Consume result with calling get_result until it returns nothing
    EXPECT_CALL(m.connection, get_result())
        .InSequence(s)
        .WillOnce(Return(boost::none));

    ozo::impl::async_get_result(m.ctx, process_f);
}

TEST_F(async_get_result, should_post_callback_with_error_and_consume_result_if_result_status_is_PGRES_EMPTY_QUERY) {
    Sequence s;
    // Post self to strand
    EXPECT_CALL(m.executor, post(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.strand, dispatch(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.callback, context_preserved()).InSequence(s);

    // Get result since is_busy() is false
    EXPECT_CALL(m.connection, is_busy()).InSequence(s).WillOnce(Return(false));
    EXPECT_CALL(m.connection, get_result())
        .InSequence(s)
        .WillOnce(Return(make_pg_result(PGRES_EMPTY_QUERY, error_code{})));

    // Post callback with error and cancel all io
    EXPECT_CALL(m.socket, cancel(_)).WillOnce(Return());
    EXPECT_CALL(m.executor, post(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.strand, dispatch(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.callback, context_preserved()).InSequence(s).WillOnce(Return());
    EXPECT_CALL(m.callback, call(error_code{ozo::error::result_status_empty_query}, _))
        .InSequence(s)
        .WillOnce(Return());

    // Consume result with calling get_result until it returns nothing
    EXPECT_CALL(m.connection, get_result())
        .InSequence(s)
        .WillOnce(Return(boost::none));

    ozo::impl::async_get_result(m.ctx, process_f);
}

TEST_F(async_get_result, should_post_callback_with_error_from_result_and_consume_result_if_result_status_is_PGRES_FATAL_ERROR) {
    Sequence s;
    // Post self to strand
    EXPECT_CALL(m.executor, post(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.strand, dispatch(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.callback, context_preserved()).InSequence(s).WillOnce(Return());

    // get result since is_busy() is false
    EXPECT_CALL(m.connection, is_busy()).InSequence(s).WillOnce(Return(false));
    EXPECT_CALL(m.connection, get_result())
        .InSequence(s)
        .WillOnce(Return(make_pg_result(PGRES_FATAL_ERROR, error_code{error::error})));

    // Post callback with error and cancel all io
    EXPECT_CALL(m.socket, cancel(_)).WillOnce(Return());
    EXPECT_CALL(m.executor, post(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.strand, dispatch(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.callback, context_preserved()).InSequence(s).WillOnce(Return());
    EXPECT_CALL(m.callback, call(error_code{error::error}, _)).InSequence(s).WillOnce(Return());

    //Consume result with calling get_result until it returns nothing
    EXPECT_CALL(m.connection, get_result())
        .InSequence(s)
        .WillOnce(Return(boost::none));

    ozo::impl::async_get_result(m.ctx, process_f);
}

struct async_get_result_ : async_get_result,
        WithParamInterface<ExecStatusType> {
};

TEST_P(async_get_result_, should_post_callback_with_error_from_result_and_consume_result) {
    Sequence s;
    // Post self to strand
    EXPECT_CALL(m.executor, post(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.strand, dispatch(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.callback, context_preserved()).InSequence(s).WillOnce(Return());

    // get result since is_busy() is false
    EXPECT_CALL(m.connection, is_busy()).InSequence(s).WillOnce(Return(false));
    EXPECT_CALL(m.connection, get_result())
        .InSequence(s)
        .WillOnce(Return(make_pg_result(GetParam(), error_code{})));

    // Post callback with error and cancel all io
    EXPECT_CALL(m.socket, cancel(_)).WillOnce(Return());
    EXPECT_CALL(m.executor, post(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.strand, dispatch(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(m.callback, context_preserved()).InSequence(s).WillOnce(Return());
    EXPECT_CALL(m.callback, call(error_code{ozo::error::result_status_unexpected}, _))
        .InSequence(s)
        .WillOnce(Return());

    //Consume result with calling get_result until it returns nothing
    EXPECT_CALL(m.connection, get_result())
        .InSequence(s)
        .WillOnce(Return(boost::none));

    ozo::impl::async_get_result(m.ctx, process_f);
}

INSTANTIATE_TEST_CASE_P(
    with_unexpected_result_status,
    async_get_result_,
    Values(PGRES_COPY_OUT, PGRES_COPY_IN, PGRES_COPY_BOTH, PGRES_NONFATAL_ERROR));

} // namespace
