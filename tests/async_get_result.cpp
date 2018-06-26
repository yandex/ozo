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

using ozo::impl::query_state;
using namespace testing;
using ozo::error_code;

GTEST("ozo::async_get_result_op::perform()") {
    fixture m;

    SHOULD("post continuation within strand") {
        EXPECT_INVOKE(m.io_context, post, _).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.strand, dispatch, _);

        ozo::impl::make_async_get_result_op(m.ctx, [](auto, auto){}).perform();
    }

    SHOULD("preserve query state") {
        EXPECT_INVOKE(m.io_context, post, _);

        ozo::impl::make_async_get_result_op(m.ctx, [](auto, auto){}).perform();

        EXPECT_EQ(m.ctx->state, query_state::send_in_progress);
    }
}

GTEST("ozo::async_get_result_op::operator()", "with query state is error",
        Values(error_code{}, error_code{error::error})) {

    fixture m;
    m.ctx->state=ozo::impl::query_state::error;

    SHOULD("exit immediately") {
        ozo::impl::make_async_get_result_op(m.ctx, [](auto, auto){})(GetParam());
    }

    SHOULD("preserve query state") {
        ozo::impl::make_async_get_result_op(m.ctx, [](auto, auto){})(GetParam());
        EXPECT_EQ(m.ctx->state, query_state::error);
    }
}

GTEST("ozo::async_get_result_op::operator(error)", "with query state is NOT error",
        Values(query_state::send_in_progress, query_state::send_finish)) {

    fixture m;
    m.ctx->state=GetParam();

    SHOULD("post callback with given error") {
        EXPECT_INVOKE(m.socket, cancel, _);
        EXPECT_INVOKE(m.io_context, post, _).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.strand, dispatch, _).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.callback, context_preserved);
        EXPECT_INVOKE(m.callback, call, error_code{error::error}, _);

        ozo::impl::make_async_get_result_op(m.ctx, [](auto, auto){})(error::error);
    }

    SHOULD("post callback with operation_aborted if called with bad_descriptor") {
        EXPECT_INVOKE(m.socket, cancel, _);
        EXPECT_INVOKE(m.io_context, post, _).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.strand, dispatch, _).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.callback, context_preserved);
        EXPECT_INVOKE(m.callback, call, error_code{boost::asio::error::operation_aborted}, _);

        ozo::impl::make_async_get_result_op(m.ctx, [](auto, auto){})(boost::asio::error::bad_descriptor);
    }

    SHOULD("set query state in error") {
        EXPECT_INVOKE(m.socket, cancel, _);
        EXPECT_INVOKE(m.io_context, post, _);

        ozo::impl::make_async_get_result_op(m.ctx, [](auto, auto){})(error::error);

        EXPECT_EQ(m.ctx->state, query_state::error);
    }
}

GTEST("ozo::async_get_result()") {
    fixture m;
    struct process_mock {
        virtual void call() const = 0;
        virtual ~process_mock() = default;
    };
    StrictGMock<process_mock> process;
    const auto process_f = [&](auto, auto){
        static_cast<process_mock&>(object(process)).call();
    };
    SHOULD("wait for read and consume input while is_busy() returns true") {
        Sequence s;
        // Post self to strand
        EXPECT_INVOKE(m.io_context, post, _).InSequence(s).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.strand, dispatch, _).InSequence(s).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.callback, context_preserved).InSequence(s);

        // wait for read while is_busy() returns true
        EXPECT_INVOKE(m.connection, is_busy).InSequence(s).WillOnce(Return(true));
        EXPECT_INVOKE(m.socket, async_read_some, _).InSequence(s).WillOnce(InvokeArgument<0>(error_code{}));
        EXPECT_INVOKE(m.strand, dispatch, _).InSequence(s).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.callback, context_preserved).InSequence(s);

        // consume input
        EXPECT_INVOKE(m.connection, consume_input).InSequence(s).WillOnce(Return(1));
        // wait for read while is_busy() which returns true
        EXPECT_INVOKE(m.connection, is_busy).InSequence(s).WillOnce(Return(true));
        EXPECT_INVOKE(m.socket, async_read_some, _).InSequence(s);

        ozo::impl::async_get_result(m.ctx, process_f);
    }

    SHOULD("post callback with error if consume input failed") {
        Sequence s;

        // Post self to strand
        EXPECT_INVOKE(m.io_context, post, _).InSequence(s).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.strand, dispatch, _).InSequence(s).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.callback, context_preserved).InSequence(s);

        // Wait for read while is_busy() returns true
        EXPECT_INVOKE(m.connection, is_busy).InSequence(s).WillOnce(Return(true));
        EXPECT_INVOKE(m.socket, async_read_some, _).InSequence(s).WillOnce(InvokeArgument<0>(error_code{}));
        EXPECT_INVOKE(m.strand, dispatch, _).InSequence(s).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.callback, context_preserved).InSequence(s);

        // Consume input
        EXPECT_INVOKE(m.connection, consume_input).InSequence(s).WillOnce(Return(0));

        // Cancel all io
        EXPECT_INVOKE(m.socket, cancel, _);

        // Post callback with error
        EXPECT_INVOKE(m.io_context, post, _).InSequence(s).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.strand, dispatch, _).InSequence(s).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.callback, context_preserved).InSequence(s);
        EXPECT_INVOKE(m.callback, call, error_code{ozo::error::pg_consume_input_failed}, _)
            .InSequence(s);

        ozo::impl::async_get_result(m.ctx, process_f);
    }

    SHOULD("process data, post callback if result is empty") {
        Sequence s;

        // Post self to strand
        EXPECT_INVOKE(m.io_context, post, _).InSequence(s).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.strand, dispatch, _).InSequence(s).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.callback, context_preserved).InSequence(s);

        // get result since is_busy() is false
        EXPECT_INVOKE(m.connection, is_busy).InSequence(s).WillOnce(Return(false));
        EXPECT_INVOKE(m.connection, get_result)
            .InSequence(s)
            .WillOnce(Return(boost::none));

        // Post callback with no error since result is empty
        EXPECT_INVOKE(m.io_context, post, _).InSequence(s).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.strand, dispatch, _).InSequence(s).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.callback, context_preserved).InSequence(s);
        EXPECT_INVOKE(m.callback, call, error_code{}, _).InSequence(s);

        ozo::impl::async_get_result(m.ctx, process_f);
    }

    SHOULD("post callback with error and consume if process data throws") {
        Sequence s;

        // Post self to strand
        EXPECT_INVOKE(m.io_context, post, _).InSequence(s).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.strand, dispatch, _).InSequence(s).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.callback, context_preserved).InSequence(s);

        // get result since is_busy() is false
        EXPECT_INVOKE(m.connection, is_busy).InSequence(s).WillOnce(Return(false));
        EXPECT_INVOKE(m.connection, get_result)
            .InSequence(s)
            .WillOnce(Return(make_pg_result(PGRES_TUPLES_OK, error_code{})));

        EXPECT_INVOKE(process, call).InSequence(s).WillOnce(Invoke([]{ throw std::runtime_error("");}));

        EXPECT_INVOKE(m.socket, cancel, _).InSequence(s);
        // Post callback with no error since result is error
        EXPECT_INVOKE(m.io_context, post, _).InSequence(s).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.strand, dispatch, _).InSequence(s).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.callback, context_preserved).InSequence(s);
        EXPECT_INVOKE(m.callback, call, error_code{ozo::error::bad_result_process}, _).InSequence(s);

        //Consume result with calling get_result until it returns nothing
        EXPECT_INVOKE(m.connection, get_result)
            .InSequence(s)
            .WillOnce(Return(boost::none));

        ozo::impl::async_get_result(m.ctx, process_f);
    }

    SHOULD("process data, post callback and consume if result status is PGRES_TUPLES_OK") {
        Sequence s;

        // Post self to strand
        EXPECT_INVOKE(m.io_context, post, _).InSequence(s).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.strand, dispatch, _).InSequence(s).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.callback, context_preserved).InSequence(s);

        // get result since is_busy() is false
        EXPECT_INVOKE(m.connection, is_busy).InSequence(s).WillOnce(Return(false));
        EXPECT_INVOKE(m.connection, get_result)
            .InSequence(s)
            .WillOnce(Return(make_pg_result(PGRES_TUPLES_OK, error_code{})));

        EXPECT_INVOKE(process, call).InSequence(s);
        // Post callback with no error since result is ok
        EXPECT_INVOKE(m.io_context, post, _).InSequence(s).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.strand, dispatch, _).InSequence(s).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.callback, context_preserved).InSequence(s);
        EXPECT_INVOKE(m.callback, call, error_code{}, _).InSequence(s);

        //Consume result with calling get_result until it returns nothing
        EXPECT_INVOKE(m.connection, get_result)
            .InSequence(s)
            .WillOnce(Return(boost::none));

        ozo::impl::async_get_result(m.ctx, process_f);
    }

    SHOULD("process data and post callback if result status is PGRES_SINGLE_TUPLE") {
        Sequence s;

        // Post self to strand
        EXPECT_INVOKE(m.io_context, post, _).InSequence(s).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.strand, dispatch, _).InSequence(s).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.callback, context_preserved).InSequence(s);

        // Get result since is_busy() is false
        EXPECT_INVOKE(m.connection, is_busy).InSequence(s).WillOnce(Return(false));
        EXPECT_INVOKE(m.connection, get_result)
            .InSequence(s)
            .WillOnce(Return(make_pg_result(PGRES_SINGLE_TUPLE, error_code{})));

        // Process result
        EXPECT_INVOKE(process, call).InSequence(s);

        // Post callback with no error since result is ok
        EXPECT_INVOKE(m.io_context, post, _).InSequence(s).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.strand, dispatch, _).InSequence(s).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.callback, context_preserved).InSequence(s);
        EXPECT_INVOKE(m.callback, call, error_code{}, _).InSequence(s);

        ozo::impl::async_get_result(m.ctx, process_f);
    }

    SHOULD("post callback and consume result if result status is PGRES_COMMAND_OK") {
        Sequence s;
        // Post self to strand
        EXPECT_INVOKE(m.io_context, post, _).InSequence(s).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.strand, dispatch, _).InSequence(s).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.callback, context_preserved).InSequence(s);

        // Get result since is_busy() is false
        EXPECT_INVOKE(m.connection, is_busy).InSequence(s).WillOnce(Return(false));
        EXPECT_INVOKE(m.connection, get_result)
            .InSequence(s)
            .WillOnce(Return(make_pg_result(PGRES_COMMAND_OK, error_code{})));

        // Post callback with no error since result is ok
        EXPECT_INVOKE(m.io_context, post, _).InSequence(s).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.strand, dispatch, _).InSequence(s).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.callback, context_preserved).InSequence(s);
        EXPECT_INVOKE(m.callback, call, error_code{}, _).InSequence(s);

        // Consume result with calling get_result until it returns nothing
        EXPECT_INVOKE(m.connection, get_result)
            .InSequence(s)
            .WillOnce(Return(boost::none));

        ozo::impl::async_get_result(m.ctx, process_f);
    }

    SHOULD("post callback with error and consume result if result status is PGRES_BAD_RESPONSE") {
        Sequence s;
        // Post self to strand
        EXPECT_INVOKE(m.io_context, post, _).InSequence(s).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.strand, dispatch, _).InSequence(s).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.callback, context_preserved).InSequence(s);

        // get result since is_busy() is false
        EXPECT_INVOKE(m.connection, is_busy).InSequence(s).WillOnce(Return(false));
        EXPECT_INVOKE(m.connection, get_result)
            .InSequence(s)
            .WillOnce(Return(make_pg_result(PGRES_BAD_RESPONSE, error_code{})));

        // Post callback with error and cancel all io
        EXPECT_INVOKE(m.socket, cancel, _);
        EXPECT_INVOKE(m.io_context, post, _).InSequence(s).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.strand, dispatch, _).InSequence(s).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.callback, context_preserved).InSequence(s);
        EXPECT_INVOKE(m.callback, call, error_code{ozo::error::result_status_bad_response}, _).InSequence(s);

        // Consume result with calling get_result until it returns nothing
        EXPECT_INVOKE(m.connection, get_result)
            .InSequence(s)
            .WillOnce(Return(boost::none));

        ozo::impl::async_get_result(m.ctx, process_f);
    }

    SHOULD("post callback with error and consume result if result status is PGRES_EMPTY_QUERY") {
        Sequence s;
        // Post self to strand
        EXPECT_INVOKE(m.io_context, post, _).InSequence(s).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.strand, dispatch, _).InSequence(s).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.callback, context_preserved).InSequence(s);

        // Get result since is_busy() is false
        EXPECT_INVOKE(m.connection, is_busy).InSequence(s).WillOnce(Return(false));
        EXPECT_INVOKE(m.connection, get_result)
            .InSequence(s)
            .WillOnce(Return(make_pg_result(PGRES_EMPTY_QUERY, error_code{})));

        // Post callback with error and cancel all io
        EXPECT_INVOKE(m.socket, cancel, _);
        EXPECT_INVOKE(m.io_context, post, _).InSequence(s).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.strand, dispatch, _).InSequence(s).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.callback, context_preserved).InSequence(s);
        EXPECT_INVOKE(m.callback, call, error_code{ozo::error::result_status_empty_query}, _).InSequence(s);

        // Consume result with calling get_result until it returns nothing
        EXPECT_INVOKE(m.connection, get_result)
            .InSequence(s)
            .WillOnce(Return(boost::none));

        ozo::impl::async_get_result(m.ctx, process_f);
    }

    SHOULD("post callback with error from result and consume result if result status is PGRES_FATAL_ERROR") {
        Sequence s;
        // Post self to strand
        EXPECT_INVOKE(m.io_context, post, _).InSequence(s).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.strand, dispatch, _).InSequence(s).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.callback, context_preserved).InSequence(s);

        // get result since is_busy() is false
        EXPECT_INVOKE(m.connection, is_busy).InSequence(s).WillOnce(Return(false));
        EXPECT_INVOKE(m.connection, get_result)
            .InSequence(s)
            .WillOnce(Return(make_pg_result(PGRES_FATAL_ERROR, error_code{error::error})));

        // Post callback with error and cancel all io
        EXPECT_INVOKE(m.socket, cancel, _);
        EXPECT_INVOKE(m.io_context, post, _).InSequence(s).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.strand, dispatch, _).InSequence(s).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.callback, context_preserved).InSequence(s);
        EXPECT_INVOKE(m.callback, call, error_code{error::error}, _).InSequence(s);

        //Consume result with calling get_result until it returns nothing
        EXPECT_INVOKE(m.connection, get_result)
            .InSequence(s)
            .WillOnce(Return(boost::none));

        ozo::impl::async_get_result(m.ctx, process_f);
    }
}

GTEST("ozo::async_get_result()", "with unexpected result status",
        Values(PGRES_COPY_OUT, PGRES_COPY_IN, PGRES_COPY_BOTH, PGRES_NONFATAL_ERROR)) {
    fixture m;
    struct process_mock {
        virtual void call() const = 0;
        virtual ~process_mock() = default;
    };
    StrictGMock<process_mock> process;
    const auto process_f = [&](auto, auto){
        static_cast<process_mock&>(object(process)).call();
    };

    SHOULD("post callback with error from result and consume result") {
        Sequence s;
        // Post self to strand
        EXPECT_INVOKE(m.io_context, post, _).InSequence(s).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.strand, dispatch, _).InSequence(s).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.callback, context_preserved).InSequence(s);

        // get result since is_busy() is false
        EXPECT_INVOKE(m.connection, is_busy).InSequence(s).WillOnce(Return(false));
        EXPECT_INVOKE(m.connection, get_result)
            .InSequence(s)
            .WillOnce(Return(make_pg_result(GetParam(), error_code{})));

        // Post callback with error and cancel all io
        EXPECT_INVOKE(m.socket, cancel, _);
        EXPECT_INVOKE(m.io_context, post, _).InSequence(s).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.strand, dispatch, _).InSequence(s).WillOnce(InvokeArgument<0>());
        EXPECT_INVOKE(m.callback, context_preserved).InSequence(s);
        EXPECT_INVOKE(m.callback, call, error_code{ozo::error::result_status_unexpected}, _)
            .InSequence(s);

        //Consume result with calling get_result until it returns nothing
        EXPECT_INVOKE(m.connection, get_result)
            .InSequence(s)
            .WillOnce(Return(boost::none));

        ozo::impl::async_get_result(m.ctx, process_f);
    }
}
} // namespace
