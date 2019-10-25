#include <connection_mock.h>
#include <test_error.h>

#include <ozo/impl/async_connect.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace ozo::tests {

struct custom_type {};

} // namespace ozo::tests

OZO_PG_DEFINE_CUSTOM_TYPE(ozo::tests::custom_type, "custom_type")

namespace {

namespace asio = boost::asio;

using namespace testing;
using namespace ozo::tests;

using ozo::empty_oid_map;

struct fixture {
    StrictMock<connection_gmock> connection{};
    StrictMock<executor_mock> strand{};
    StrictMock<stream_descriptor_mock> socket{};
    StrictMock<steady_timer_mock> timer{};
    io_context io;
    decltype(make_connection(connection, io, socket)) conn =
            make_connection(connection, io, socket);
    StrictMock<executor_mock> callback_executor{};
    StrictMock<callback_gmock<decltype(conn)>> callback{};

    auto async_connect_op() {
        return ozo::impl::async_connect_op(conn, wrap(callback));
    }

    static auto make_native_handle(native_handle::state state = native_handle::good) {
        return std::make_shared<native_handle>(state);
    }

    fixture() {
        EXPECT_CALL(io.strand_service_, get_executor()).WillOnce(ReturnRef(strand));
        auto ex = boost::asio::executor(ozo::detail::make_strand_executor(io.get_executor()));
        EXPECT_CALL(callback, get_executor()).WillRepeatedly(Return(ex));
    }
};

using ozo::error_code;
using ozo::time_traits;

struct async_connect_op : Test {};

TEST_F(async_connect_op, should_start_connection_assign_and_wait_for_compile) {
    fixture f;

    const InSequence s;

    EXPECT_CALL(f.connection, start_connection("conninfo")).WillOnce(Return(f.make_native_handle()));
    EXPECT_CALL(f.connection, assign()).WillOnce(Return(error_code{}));
    EXPECT_CALL(f.socket, async_write_some(_)).WillOnce(Return());

    f.async_connect_op().perform("conninfo");
}

TEST_F(async_connect_op, should_call_handler_with_pq_connection_start_failed_on_nullptr_in_start_connection) {
    fixture f;

    const InSequence s;

    EXPECT_CALL(f.connection, start_connection("conninfo"))
        .WillOnce(Return(std::shared_ptr<native_handle>{}));

    EXPECT_CALL(f.callback, call(error_code{ozo::error::pq_connection_start_failed}, f.conn))
        .WillOnce(Return());

    f.async_connect_op().perform("conninfo");
}

TEST_F(async_connect_op, should_call_handler_with_pq_connection_status_bad_if_connection_status_is_bad) {
    fixture f;

    const InSequence s;

    EXPECT_CALL(f.connection, start_connection("conninfo")).WillOnce(Return(f.make_native_handle(native_handle::bad)));

    EXPECT_CALL(f.callback, call(error_code{ozo::error::pq_connection_status_bad}, f.conn))
        .WillOnce(Return());

    f.async_connect_op().perform("conninfo");
}

TEST_F(async_connect_op, should_call_handler_with_error_if_assign_returns_error) {
    fixture f;

    const InSequence s;

    EXPECT_CALL(f.connection, start_connection("conninfo")).WillOnce(Return(f.make_native_handle()));
    EXPECT_CALL(f.connection, assign()).WillOnce(Return(error_code{error::error}));

    EXPECT_CALL(f.callback, call(error_code{error::error}, f.conn)).WillOnce(Return());

    f.async_connect_op().perform("conninfo");
}

TEST_F(async_connect_op, should_wait_for_write_complete_if_connect_poll_returns_PGRES_POLLING_WRITING) {
    fixture f;

    const InSequence s;

    EXPECT_CALL(f.connection, start_connection("conninfo")).WillOnce(Return(f.make_native_handle()));
    EXPECT_CALL(f.connection, assign()).WillOnce(Return(error_code{}));

    EXPECT_CALL(f.socket, async_write_some(_)).WillOnce(InvokeArgument<0>(error_code{}));

    EXPECT_CALL(f.strand, post(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(f.connection, connect_poll()).WillOnce(Return(PGRES_POLLING_WRITING));

    EXPECT_CALL(f.socket, async_write_some(_)).WillOnce(Return());

    f.async_connect_op().perform("conninfo");
}

TEST_F(async_connect_op, should_wait_for_read_complete_if_connect_poll_returns_PGRES_POLLING_READING) {
    fixture f;

    const InSequence s;

    EXPECT_CALL(f.connection, start_connection("conninfo")).WillOnce(Return(f.make_native_handle()));
    EXPECT_CALL(f.connection, assign()).WillOnce(Return(error_code{}));

    EXPECT_CALL(f.socket, async_write_some(_)).WillOnce(InvokeArgument<0>(error_code{}));

    EXPECT_CALL(f.strand, post(_)).WillOnce(InvokeArgument<0>());

    EXPECT_CALL(f.connection, connect_poll()).WillOnce(Return(PGRES_POLLING_READING));

    EXPECT_CALL(f.socket, async_read_some(_)).WillOnce(Return());

    f.async_connect_op().perform("conninfo");
}

TEST_F(async_connect_op, should_call_handler_with_no_error_if_connect_poll_returns_PGRES_POLLING_OK) {
    fixture f;

    const InSequence s;

    EXPECT_CALL(f.connection, start_connection("conninfo")).WillOnce(Return(f.make_native_handle()));
    EXPECT_CALL(f.connection, assign()).WillOnce(Return(error_code{}));

    EXPECT_CALL(f.socket, async_write_some(_)).WillOnce(InvokeArgument<0>(error_code{}));

    EXPECT_CALL(f.strand, post(_)).WillOnce(InvokeArgument<0>());

    EXPECT_CALL(f.connection, connect_poll()).WillOnce(Return(PGRES_POLLING_OK));

    EXPECT_CALL(f.callback, call(error_code{}, f.conn)).WillOnce(Return());

    f.async_connect_op().perform("conninfo");
}

TEST_F(async_connect_op, should_call_handler_with_pq_connect_poll_failed_if_connect_poll_returns_PGRES_POLLING_FAILED) {
    fixture f;

    const InSequence s;

    EXPECT_CALL(f.connection, start_connection("conninfo")).WillOnce(Return(f.make_native_handle()));
    EXPECT_CALL(f.connection, assign()).WillOnce(Return(error_code{}));

    EXPECT_CALL(f.socket, async_write_some(_)).WillOnce(InvokeArgument<0>(error_code{}));

    EXPECT_CALL(f.strand, post(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(f.connection, connect_poll()).WillOnce(Return(PGRES_POLLING_FAILED));

    EXPECT_CALL(f.callback, call(error_code{ozo::error::pq_connect_poll_failed}, f.conn))
        .WillOnce(Return());

    f.async_connect_op().perform("conninfo");
}

TEST_F(async_connect_op, should_call_handler_with_pq_connect_poll_failed_if_connect_poll_returns_PGRES_POLLING_ACTIVE) {
    fixture f;

    const InSequence s;

    EXPECT_CALL(f.connection, start_connection("conninfo")).WillOnce(Return(f.make_native_handle()));
    EXPECT_CALL(f.connection, assign()).WillOnce(Return(error_code{}));

    EXPECT_CALL(f.socket, async_write_some(_)).WillOnce(InvokeArgument<0>(error_code{}));

    EXPECT_CALL(f.strand, post(_)).WillOnce(InvokeArgument<0>());

    EXPECT_CALL(f.connection, connect_poll()).WillOnce(Return(PGRES_POLLING_ACTIVE));

    EXPECT_CALL(f.callback, call(error_code{ozo::error::pq_connect_poll_failed}, f.conn))
        .WillOnce(Return());

    f.async_connect_op().perform("conninfo");
}

TEST_F(async_connect_op, should_call_handler_with_the_error_if_polling_operation_invokes_callback_with_it) {
    fixture f;

    const InSequence s;

    EXPECT_CALL(f.connection, start_connection("conninfo")).WillOnce(Return(f.make_native_handle()));
    EXPECT_CALL(f.connection, assign()).WillOnce(Return(error_code{}));

    EXPECT_CALL(f.socket, async_write_some(_)).WillOnce(InvokeArgument<0>(error::error));

    EXPECT_CALL(f.strand, post(_)).WillOnce(InvokeArgument<0>());

    EXPECT_CALL(f.callback, call(error_code{error::error}, f.conn))
        .WillOnce(Return());

    f.async_connect_op().perform("conninfo");
}

struct async_connect_op_call : Test {};

TEST_F(async_connect_op_call, should_replace_empty_connection_error_context_on_error) {
    fixture f;

    EXPECT_CALL(f.callback, call(error_code{error::error}, f.conn))
        .WillOnce(Return());

    f.async_connect_op()(error_code{error::error});

    EXPECT_EQ(f.conn->error_context_, "error while connection polling");
}

TEST_F(async_connect_op_call, should_preserve_not_empty_connection_error_context_on_error) {
    fixture f;
    f.conn->error_context_ = "my error";

    EXPECT_CALL(f.callback, call(error_code{error::error}, f.conn))
        .WillOnce(Return());

    f.async_connect_op()(error_code{error::error});

    EXPECT_EQ(f.conn->error_context_, "my error");
}

struct async_connect : Test {
    fixture f;
};

TEST_F(async_connect, should_cancel_timer_when_operation_is_done_before_timeout) {
    StrictMock<callback_gmock<decltype(f.conn)>> callback{};
    execution_context cb_io;
    EXPECT_CALL(callback, get_executor()).WillRepeatedly(Return(cb_io.get_executor()));
    EXPECT_CALL(f.io.strand_service_, get_executor()).WillRepeatedly(ReturnRef(f.strand));
    EXPECT_CALL(f.io.timer_service_, timer(time_traits::duration(42))).WillRepeatedly(ReturnRef(f.timer));

    std::function<void (ozo::error_code)> on_timer_expired;
    std::function<void (ozo::error_code)> on_async_write_some;

    EXPECT_CALL(f.timer, async_wait(_)).WillOnce(SaveArg<0>(&on_timer_expired));

    Sequence s;

    EXPECT_CALL(f.connection, start_connection("conninfo")).InSequence(s).WillOnce(Return(f.make_native_handle()));
    EXPECT_CALL(f.connection, assign()).InSequence(s).WillOnce(Return(error_code{}));


    EXPECT_CALL(f.socket, async_write_some(_)).InSequence(s).WillOnce(SaveArg<0>(&on_async_write_some));
    EXPECT_CALL(f.strand, post(_)).InSequence(s).WillOnce(InvokeArgument<0>());

    EXPECT_CALL(f.connection, connect_poll()).InSequence(s).WillOnce(Return(PGRES_POLLING_OK));

    EXPECT_CALL(f.timer, cancel()).InSequence(s).WillOnce(Return(1));

    EXPECT_CALL(f.strand, post(_)).InSequence(s).WillOnce(InvokeArgument<0>());

    EXPECT_CALL(cb_io.executor_, dispatch(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(callback, call(error_code{}, f.conn)).InSequence(s).WillOnce(Return());

    ozo::impl::async_connect("conninfo", time_traits::duration(42), f.conn, wrap(callback));

    on_async_write_some(ozo::error_code{});
    on_timer_expired(boost::asio::error::operation_aborted);
}

TEST_F(async_connect, should_cancel_socket_on_timeout) {
    StrictMock<callback_gmock<decltype(f.conn)>> callback{};
    execution_context cb_io;
    std::function<void (error_code)> on_timer_expired;
    std::function<void (error_code)> on_async_write_some;

    EXPECT_CALL(callback, get_executor()).WillRepeatedly(Return(cb_io.get_executor()));
    EXPECT_CALL(f.io.strand_service_, get_executor()).WillRepeatedly(ReturnRef(f.strand));
    EXPECT_CALL(f.io.timer_service_, timer(time_traits::duration(42))).WillRepeatedly(ReturnRef(f.timer));
    EXPECT_CALL(f.timer, async_wait(_)).WillOnce(SaveArg<0>(&on_timer_expired));

    Sequence s;

    EXPECT_CALL(f.connection, start_connection("conninfo")).InSequence(s).WillOnce(Return(f.make_native_handle()));
    EXPECT_CALL(f.connection, assign()).InSequence(s).WillOnce(Return(error_code{}));
    EXPECT_CALL(f.socket, async_write_some(_)).InSequence(s).WillOnce(SaveArg<0>(&on_async_write_some));
    EXPECT_CALL(f.strand, post(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(f.socket, cancel(_)).InSequence(s).WillOnce(Return());

    EXPECT_CALL(f.strand, post(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(cb_io.executor_, dispatch(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(callback, call(Eq(boost::asio::error::timed_out), f.conn)).InSequence(s).WillOnce(Return());

    ozo::impl::async_connect("conninfo", time_traits::duration(42), f.conn, wrap(callback));

    on_timer_expired(error_code {});
    on_async_write_some(boost::asio::error::operation_aborted);
}

TEST_F(async_connect, should_request_oid_map_when_oid_map_is_not_empty) {
    auto conn = make_connection(f.connection, f.io, f.socket, ozo::register_types<custom_type>());
    StrictMock<callback_gmock<decltype(conn)>> callback {};

    execution_context cb_io;
    EXPECT_CALL(callback, get_executor()).WillRepeatedly(Return(cb_io.get_executor()));
    EXPECT_CALL(f.io.strand_service_, get_executor()).WillRepeatedly(ReturnRef(f.strand));
    EXPECT_CALL(f.io.timer_service_, timer(time_traits::duration(42))).WillRepeatedly(ReturnRef(f.timer));
    EXPECT_CALL(f.timer, async_wait(_)).WillOnce(Return());

    Sequence s;

    EXPECT_CALL(f.connection, start_connection("conninfo")).InSequence(s).WillOnce(Return(f.make_native_handle()));
    EXPECT_CALL(f.connection, assign()).InSequence(s).WillOnce(Return(error_code{}));

    EXPECT_CALL(f.socket, async_write_some(_)).InSequence(s).WillOnce(InvokeArgument<0>(error_code{}));

    EXPECT_CALL(f.strand, post(_)).InSequence(s).WillOnce(InvokeArgument<0>());

    EXPECT_CALL(f.connection, connect_poll()).InSequence(s).WillOnce(Return(PGRES_POLLING_OK));

    EXPECT_CALL(f.connection, request_oid_map()).InSequence(s).WillOnce(Return());

    ozo::impl::async_connect("conninfo", time_traits::duration(42), conn, wrap(callback));
}

} // namespace
