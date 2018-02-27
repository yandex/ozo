#include <ozo/impl/async_request.h>
#include "test_error.h"
#include "connection_mock.h"
#include <GUnit/GTest.h>

namespace {

namespace hana = ::boost::hana;
using namespace ozo::testing;
using callback_mock = ozo::testing::callback_mock<connection_ptr<>>;
struct fixture {
    testing::StrictGMock<connection_mock> connection{};
    testing::StrictGMock<callback_mock> callback{};
    testing::StrictGMock<ozo::testing::executor_mock> io_context{};
    testing::StrictGMock<ozo::testing::executor_mock> strand{};
    testing::StrictGMock<ozo::testing::strand_executor_service_mock> strand_service{};
    testing::StrictGMock<ozo::testing::stream_descriptor_mock> socket{};
    ozo::testing::io_context io{object(io_context), object(strand_service)};
    decltype(make_connection(object(connection), io, object(socket))) conn =
            make_connection(object(connection), io, object(socket));

    auto make_operation_context() {
        using namespace testing;
        EXPECT_CALL(strand_service, (get_executor)()).WillOnce(ReturnRef(strand));
        return ozo::impl::make_operation_context(conn, wrap(callback));
    }

    decltype(ozo::impl::make_operation_context(conn, wrap(callback))) ctx;

    fixture() : ctx(make_operation_context()) {}

};

GTEST("ozo::async_send_query_params_op::perform()") {
    using namespace testing;
    using ozo::error_code;

    SHOULD("set non blocking mode, send query and params and post continuation in strand") {
        fixture m;

        EXPECT_CALL(m.connection, (set_nonblocking)()).WillOnce(Return(0));
        EXPECT_CALL(m.connection, (send_query_params)()).WillOnce(Return(1));
        EXPECT_CALL(m.io_context, (post)(_)).WillOnce(InvokeArgument<0>());
        EXPECT_CALL(m.strand, (dispatch)(_));

        ozo::impl::make_async_send_query_params_op(m.ctx, fake_query{}).perform();

        EXPECT_EQ(m.ctx->state, ozo::impl::query_state::send_in_progress);
    }

    SHOULD("set error state, cancel io and invoke callback with pg_set_nonblocking_failed if pg_set_nonbloking returns -1") {
        fixture m;

        EXPECT_CALL(m.connection, (set_nonblocking)()).WillOnce(Return(-1));
        EXPECT_INVOKE(m.socket, cancel, _);
        EXPECT_CALL(m.io_context, (post)(_)).WillOnce(InvokeArgument<0>());
        EXPECT_CALL(m.strand, (dispatch)(_)).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.callback, context_preserved);
        EXPECT_INVOKE(m.callback, call, error_code{ozo::error::pg_set_nonblocking_failed}, _);

        ozo::impl::make_async_send_query_params_op(m.ctx, fake_query{}).perform();

        EXPECT_EQ(m.ctx->state, ::ozo::impl::query_state::error);
    }

    SHOULD("call send_query_params while it returns error") {
        fixture m;

        // According to the documentation
        //   In the nonblocking state, calls to PQsendQuery, PQputline,
        //   PQputnbytes, PQputCopyData, and PQendcopy will not block
        //   but instead return an error if they need to be called again.
        // PQsendQueryParams is PQsendQuery family function so it must
        // conform to the same rules.

        Sequence s;
        EXPECT_CALL(m.connection, (set_nonblocking)()).InSequence(s).WillOnce(Return(0));
        EXPECT_CALL(m.connection, (send_query_params)()).InSequence(s).WillOnce(Return(0));
        EXPECT_CALL(m.connection, (send_query_params)()).InSequence(s).WillOnce(Return(0));
        EXPECT_CALL(m.connection, (send_query_params)()).InSequence(s).WillOnce(Return(1));
        EXPECT_CALL(m.io_context, (post)(_));

        ozo::impl::make_async_send_query_params_op(m.ctx, fake_query{}).perform();

        EXPECT_EQ(m.ctx->state, ozo::impl::query_state::send_in_progress);
    }
}

GTEST("ozo::async_send_query_params_op::operator()") {
    using namespace testing;
    using ozo::error_code;

    SHOULD("exit immediately if query state is error and called with no error") {
        fixture m;

        m.ctx->state=ozo::impl::query_state::error;

        ozo::impl::make_async_send_query_params_op(m.ctx, fake_query{})();

        EXPECT_EQ(m.ctx->state, ::ozo::impl::query_state::error);
    }

    SHOULD("exit immediately if query state is error and called with error") {
        fixture m;

        m.ctx->state=ozo::impl::query_state::error;

        ozo::impl::make_async_send_query_params_op(m.ctx, fake_query{})(::ozo::testing::error::error);

        EXPECT_EQ(m.ctx->state, ::ozo::impl::query_state::error);
    }

    SHOULD("exit immediately if query state is send_finish and called with no error") {
        fixture m;

        m.ctx->state=ozo::impl::query_state::send_finish;

        ozo::impl::make_async_send_query_params_op(m.ctx, fake_query{})();

        EXPECT_EQ(m.ctx->state, ::ozo::impl::query_state::send_finish);
    }

    SHOULD("exit immediately if query state is send_finish and called with error") {
        fixture m;

        m.ctx->state=ozo::impl::query_state::send_finish;

        ozo::impl::make_async_send_query_params_op(m.ctx, fake_query{})(::ozo::testing::error::error);

        EXPECT_EQ(m.ctx->state, ::ozo::impl::query_state::send_finish);
    }

    SHOULD("invoke callback with given error if called with error and query state is send_in_progress") {
        fixture m;

        EXPECT_INVOKE(m.socket, cancel, _);
        EXPECT_CALL(m.io_context, (post)(_)).WillOnce(InvokeArgument<0>());
        EXPECT_CALL(m.strand, (dispatch)(_)).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.callback, context_preserved);
        EXPECT_INVOKE(m.callback, call, error_code{::ozo::testing::error::error}, _);

        m.ctx->state=ozo::impl::query_state::send_in_progress;
        ozo::impl::make_async_send_query_params_op(m.ctx, fake_query{})(::ozo::testing::error::error);

        EXPECT_EQ(m.ctx->state, ::ozo::impl::query_state::error);
    }

    SHOULD("exit if flush_output() returns send_finish") {
        fixture m;

        EXPECT_CALL(m.connection, (flush_output)())
            .WillOnce(Return(ozo::impl::query_state::send_finish));

        m.ctx->state=ozo::impl::query_state::send_in_progress;
        ozo::impl::make_async_send_query_params_op(m.ctx, fake_query{})();

        EXPECT_EQ(m.ctx->state, ::ozo::impl::query_state::send_finish);
    }

    SHOULD("invoke callback with pg_flush_failed if flush_output() returns error") {
        fixture m;

        EXPECT_CALL(m.connection, (flush_output)())
            .WillOnce(Return(ozo::impl::query_state::error));
        EXPECT_INVOKE(m.socket, cancel, _);
        EXPECT_CALL(m.io_context, (post)(_)).WillOnce(InvokeArgument<0>());
        EXPECT_CALL(m.strand, (dispatch)(_)).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.callback, context_preserved);
        EXPECT_INVOKE(m.callback, call, error_code{::ozo::error::pg_flush_failed}, _);

        m.ctx->state=ozo::impl::query_state::send_in_progress;
        ozo::impl::make_async_send_query_params_op(m.ctx, fake_query{})();

        EXPECT_EQ(m.ctx->state, ::ozo::impl::query_state::error);
    }

    SHOULD("wait for write if flush_output() returns send_in_progress") {
        fixture m;

        EXPECT_CALL(m.connection, (flush_output)())
            .WillOnce(Return(ozo::impl::query_state::send_in_progress));
        EXPECT_INVOKE(m.socket, async_write_some, _);

        m.ctx->state=ozo::impl::query_state::send_in_progress;
        ozo::impl::make_async_send_query_params_op(m.ctx, fake_query{})();

        EXPECT_EQ(m.ctx->state, ::ozo::impl::query_state::send_in_progress);
    }

    SHOULD("wait for write in strand") {
        fixture m;

        Sequence s;
        EXPECT_CALL(m.connection, (flush_output)()).InSequence(s)
            .WillOnce(Return(ozo::impl::query_state::send_in_progress));
        EXPECT_INVOKE(m.socket, async_write_some, _).WillOnce(InvokeArgument<0>(error_code{}));
        EXPECT_CALL(m.strand, (dispatch)(_)).Times(AtLeast(1)).WillRepeatedly(InvokeArgument<0>());
        EXPECT_INVOKE(m.callback, context_preserved);
        EXPECT_CALL(m.connection, (flush_output)()).InSequence(s)
            .WillOnce(Return(ozo::impl::query_state::send_finish));

        m.ctx->state=ozo::impl::query_state::send_in_progress;
        ozo::impl::make_async_send_query_params_op(m.ctx, fake_query{})();
    }
}

} // namespace
