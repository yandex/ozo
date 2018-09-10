#include <connection_mock.h>
#include <test_error.h>

#include <ozo/impl/async_connect.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace ozo::tests {

struct custom_type {};

} // namespace ozo::tests

OZO_PG_DEFINE_CUSTOM_TYPE(ozo::tests::custom_type, "custom_type", dynamic_size)

namespace {

namespace asio = boost::asio;

using namespace testing;
using namespace ozo::tests;

using ozo::empty_oid_map;

struct fixture {
    StrictMock<connection_gmock> connection{};
    StrictMock<executor_gmock> executor{};
    StrictMock<executor_gmock> strand{};
    StrictMock<strand_executor_service_gmock> strand_service{};
    StrictMock<stream_descriptor_gmock> socket{};
    StrictMock<steady_timer_gmock> timer{};
    io_context io {executor, strand_service};
    decltype(make_connection(connection, io, socket, timer)) conn =
            make_connection(connection, io, socket, timer);
    StrictMock<executor_gmock> callback_executor{};
    StrictMock<callback_gmock<decltype(conn)>> callback{};

    decltype(ozo::impl::make_connect_operation_context(conn, wrap(callback))) context;

    auto make_operation_context() {
        EXPECT_CALL(strand_service, get_executor()).WillOnce(ReturnRef(strand));
        return ozo::impl::make_connect_operation_context(conn, wrap(callback));
    }

    fixture() : context(make_operation_context()) {}
};

using ozo::error_code;
using ozo::time_traits;

struct async_connect_op : Test {};

TEST_F(async_connect_op, should_start_connection_assign_socket_and_wait_for_compile) {
    fixture f;
    *(f.conn->handle_) = native_handle::good;

    const InSequence s;

    EXPECT_CALL(f.connection, start_connection("conninfo")).WillOnce(Return(error_code{}));
    EXPECT_CALL(f.connection, assign_socket()).WillOnce(Return(error_code{}));
    EXPECT_CALL(f.timer, expires_after(time_traits::duration(42))).WillOnce(Return(0));
    EXPECT_CALL(f.timer, async_wait(_)).WillOnce(Return());
    EXPECT_CALL(f.socket, async_write_some(_)).WillOnce(Return());

    ozo::impl::make_async_connect_op(f.context).perform("conninfo", time_traits::duration(42));
}

TEST_F(async_connect_op, should_call_handler_with_pq_connection_start_failed_on_error_in_start_connection) {
    fixture f;
    *(f.conn->handle_) = native_handle::good;

    const InSequence s;

    EXPECT_CALL(f.connection, start_connection("conninfo"))
        .WillOnce(Return(error_code{ozo::error::pq_connection_start_failed}));

    EXPECT_CALL(f.callback, get_executor()).WillOnce(Return(ozo::tests::executor {&f.callback_executor}));
    EXPECT_CALL(f.strand, post(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(f.callback_executor, dispatch(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(f.callback, call(error_code{ozo::error::pq_connection_start_failed}, f.conn))
        .WillOnce(Return());

    ozo::impl::make_async_connect_op(f.context).perform("conninfo", time_traits::duration(42));
}

TEST_F(async_connect_op, should_call_handler_with_pq_connection_status_bad_if_connection_status_is_bad) {
    fixture f;
    *(f.conn->handle_) = native_handle::bad;

    const InSequence s;

    EXPECT_CALL(f.connection, start_connection("conninfo")).WillOnce(Return(error_code{}));

    EXPECT_CALL(f.callback, get_executor()).WillOnce(Return(ozo::tests::executor {&f.callback_executor}));
    EXPECT_CALL(f.strand, post(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(f.callback_executor, dispatch(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(f.callback, call(error_code{ozo::error::pq_connection_status_bad}, f.conn))
        .WillOnce(Return());

    ozo::impl::make_async_connect_op(f.context).perform("conninfo", time_traits::duration(42));
}

TEST_F(async_connect_op, should_call_handler_with_error_if_assign_socket_returns_error) {
    fixture f;
    *(f.conn->handle_) = native_handle::good;

    const InSequence s;

    EXPECT_CALL(f.connection, start_connection("conninfo")).WillOnce(Return(error_code{}));
    EXPECT_CALL(f.connection, assign_socket()).WillOnce(Return(error_code{error::error}));

    EXPECT_CALL(f.callback, get_executor()).WillOnce(Return(ozo::tests::executor {&f.callback_executor}));
    EXPECT_CALL(f.strand, post(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(f.callback_executor, dispatch(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(f.callback, call(error_code{error::error}, f.conn)).WillOnce(Return());

    ozo::impl::make_async_connect_op(f.context).perform("conninfo", time_traits::duration(42));
}

TEST_F(async_connect_op, should_wait_for_write_complete_if_connect_poll_returns_PGRES_POLLING_WRITING) {
    fixture f;
    *(f.conn->handle_) = native_handle::good;

    const InSequence s;

    EXPECT_CALL(f.connection, start_connection("conninfo")).WillOnce(Return(error_code{}));
    EXPECT_CALL(f.connection, assign_socket()).WillOnce(Return(error_code{}));

    EXPECT_CALL(f.timer, expires_after(time_traits::duration(42))).WillOnce(Return(0));
    EXPECT_CALL(f.timer, async_wait(_)).WillOnce(Return());
    EXPECT_CALL(f.socket, async_write_some(_)).WillOnce(InvokeArgument<0>(error_code{}));

    EXPECT_CALL(f.strand, post(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(f.connection, connect_poll()).WillOnce(Return(PGRES_POLLING_WRITING));

    EXPECT_CALL(f.socket, async_write_some(_)).WillOnce(Return());

    ozo::impl::make_async_connect_op(f.context).perform("conninfo", time_traits::duration(42));
}

TEST_F(async_connect_op, should_wait_for_read_complete_if_connect_poll_returns_PGRES_POLLING_READING) {
    fixture f;
    *(f.conn->handle_) = native_handle::good;

    const InSequence s;

    EXPECT_CALL(f.connection, start_connection("conninfo")).WillOnce(Return(error_code{}));
    EXPECT_CALL(f.connection, assign_socket()).WillOnce(Return(error_code{}));
    EXPECT_CALL(f.timer, expires_after(time_traits::duration(42))).WillOnce(Return(0));
    EXPECT_CALL(f.timer, async_wait(_)).WillOnce(Return());

    EXPECT_CALL(f.socket, async_write_some(_)).WillOnce(InvokeArgument<0>(error_code{}));

    EXPECT_CALL(f.strand, post(_)).WillOnce(InvokeArgument<0>());

    EXPECT_CALL(f.connection, connect_poll()).WillOnce(Return(PGRES_POLLING_READING));

    EXPECT_CALL(f.socket, async_read_some(_)).WillOnce(Return());

    ozo::impl::make_async_connect_op(f.context).perform("conninfo", time_traits::duration(42));
}

TEST_F(async_connect_op, should_call_handler_with_no_error_if_connect_poll_returns_PGRES_POLLING_OK) {
    fixture f;
    *(f.conn->handle_) = native_handle::good;

    const InSequence s;

    EXPECT_CALL(f.connection, start_connection("conninfo")).WillOnce(Return(error_code{}));
    EXPECT_CALL(f.connection, assign_socket()).WillOnce(Return(error_code{}));
    EXPECT_CALL(f.timer, expires_after(time_traits::duration(42))).WillOnce(Return(0));
    EXPECT_CALL(f.timer, async_wait(_)).WillOnce(Return());

    EXPECT_CALL(f.socket, async_write_some(_)).WillOnce(InvokeArgument<0>(error_code{}));

    EXPECT_CALL(f.strand, post(_)).WillOnce(InvokeArgument<0>());

    EXPECT_CALL(f.connection, connect_poll()).WillOnce(Return(PGRES_POLLING_OK));

    EXPECT_CALL(f.callback, get_executor()).WillOnce(Return(ozo::tests::executor {&f.callback_executor}));
    EXPECT_CALL(f.strand, post(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(f.callback_executor, dispatch(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(f.callback, call(error_code{}, f.conn)).WillOnce(Return());

    ozo::impl::make_async_connect_op(f.context).perform("conninfo", time_traits::duration(42));
}

TEST_F(async_connect_op, should_call_handler_with_pq_connect_poll_failed_if_connect_poll_returns_PGRES_POLLING_FAILED) {
    fixture f;
    *(f.conn->handle_) = native_handle::good;

    const InSequence s;

    EXPECT_CALL(f.connection, start_connection("conninfo")).WillOnce(Return(error_code{}));
    EXPECT_CALL(f.connection, assign_socket()).WillOnce(Return(error_code{}));
    EXPECT_CALL(f.timer, expires_after(time_traits::duration(42))).WillOnce(Return(0));
    EXPECT_CALL(f.timer, async_wait(_)).WillOnce(Return());

    EXPECT_CALL(f.socket, async_write_some(_)).WillOnce(InvokeArgument<0>(error_code{}));

    EXPECT_CALL(f.strand, post(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(f.connection, connect_poll()).WillOnce(Return(PGRES_POLLING_FAILED));

    EXPECT_CALL(f.callback, get_executor()).WillOnce(Return(ozo::tests::executor {&f.callback_executor}));
    EXPECT_CALL(f.strand, post(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(f.callback_executor, dispatch(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(f.callback, call(error_code{ozo::error::pq_connect_poll_failed}, f.conn))
        .WillOnce(Return());

    ozo::impl::make_async_connect_op(f.context).perform("conninfo", time_traits::duration(42));
}

TEST_F(async_connect_op, should_call_handler_with_pq_connect_poll_failed_if_connect_poll_returns_PGRES_POLLING_ACTIVE) {
    fixture f;
    *(f.conn->handle_) = native_handle::good;

    const InSequence s;

    EXPECT_CALL(f.connection, start_connection("conninfo")).WillOnce(Return(error_code{}));
    EXPECT_CALL(f.connection, assign_socket()).WillOnce(Return(error_code{}));
    EXPECT_CALL(f.timer, expires_after(time_traits::duration(42))).WillOnce(Return(0));
    EXPECT_CALL(f.timer, async_wait(_)).WillOnce(Return());

    EXPECT_CALL(f.socket, async_write_some(_)).WillOnce(InvokeArgument<0>(error_code{}));

    EXPECT_CALL(f.strand, post(_)).WillOnce(InvokeArgument<0>());

    EXPECT_CALL(f.connection, connect_poll()).WillOnce(Return(PGRES_POLLING_ACTIVE));

    EXPECT_CALL(f.callback, get_executor()).WillOnce(Return(ozo::tests::executor {&f.callback_executor}));
    EXPECT_CALL(f.strand, post(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(f.callback_executor, dispatch(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(f.callback, call(error_code{ozo::error::pq_connect_poll_failed}, f.conn))
        .WillOnce(Return());

    ozo::impl::make_async_connect_op(f.context).perform("conninfo", time_traits::duration(42));
}

TEST_F(async_connect_op, should_call_handler_with_the_error_if_polling_operation_invokes_callback_with_it) {
    fixture f;
    *(f.conn->handle_) = native_handle::good;

    const InSequence s;

    EXPECT_CALL(f.connection, start_connection("conninfo")).WillOnce(Return(error_code{}));
    EXPECT_CALL(f.connection, assign_socket()).WillOnce(Return(error_code{}));
    EXPECT_CALL(f.timer, expires_after(time_traits::duration(42))).WillOnce(Return(0));
    EXPECT_CALL(f.timer, async_wait(_)).WillOnce(Return());

    EXPECT_CALL(f.socket, async_write_some(_)).WillOnce(InvokeArgument<0>(error::error));

    EXPECT_CALL(f.strand, post(_)).WillOnce(InvokeArgument<0>());

    EXPECT_CALL(f.callback, get_executor()).WillOnce(Return(ozo::tests::executor {&f.callback_executor}));
    EXPECT_CALL(f.strand, post(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(f.callback_executor, dispatch(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(f.callback, call(error_code{error::error}, f.conn))
        .WillOnce(Return());

    ozo::impl::make_async_connect_op(f.context).perform("conninfo", time_traits::duration(42));
}

TEST_F(async_connect_op, should_cancel_socket_on_timeout) {
    fixture f;
    *(f.conn->handle_) = native_handle::good;

    std::function<void (error_code)> on_timeout;

    const InSequence s;

    EXPECT_CALL(f.connection, start_connection("conninfo")).WillOnce(Return(error_code{}));
    EXPECT_CALL(f.connection, assign_socket()).WillOnce(Return(error_code{}));
    EXPECT_CALL(f.timer, expires_after(time_traits::duration(42))).WillOnce(Return(0));

    EXPECT_CALL(f.timer, async_wait(_)).WillOnce(SaveArg<0>(&on_timeout));

    EXPECT_CALL(f.socket, async_write_some(_)).WillOnce(Return());

    ozo::impl::make_async_connect_op(f.context).perform("conninfo", time_traits::duration(42));

    EXPECT_CALL(f.strand, post(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(f.socket, cancel(_)).WillOnce(Return());

    on_timeout(error_code {});
}

TEST_F(async_connect_op, should_not_cancel_socket_for_aborted_timer_async_wait) {
    fixture f;
    *(f.conn->handle_) = native_handle::good;

    std::function<void (error_code)> on_timeout;

    const InSequence s;

    EXPECT_CALL(f.connection, start_connection("conninfo")).WillOnce(Return(error_code{}));
    EXPECT_CALL(f.connection, assign_socket()).WillOnce(Return(error_code{}));
    EXPECT_CALL(f.timer, expires_after(time_traits::duration(42))).WillOnce(Return(0));
    EXPECT_CALL(f.timer, async_wait(_)).WillOnce(SaveArg<0>(&on_timeout));

    EXPECT_CALL(f.socket, async_write_some(_)).WillOnce(InvokeArgument<0>(error_code{}));

    EXPECT_CALL(f.strand, post(_)).WillOnce(InvokeArgument<0>());

    EXPECT_CALL(f.connection, connect_poll()).WillOnce(Return(PGRES_POLLING_OK));

    EXPECT_CALL(f.callback, get_executor()).WillOnce(Return(ozo::tests::executor {&f.callback_executor}));
    EXPECT_CALL(f.strand, post(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(f.callback_executor, dispatch(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(f.callback, call(error_code{}, f.conn)).WillOnce(Return());

    ozo::impl::make_async_connect_op(f.context).perform("conninfo", time_traits::duration(42));

    EXPECT_CALL(f.strand, post(_)).WillOnce(InvokeArgument<0>());

    on_timeout(error_code {boost::asio::error::operation_aborted});
}

struct async_connect_op_call : Test {};

TEST_F(async_connect_op_call, should_replace_empty_connection_error_context_on_error) {
    fixture f;

    const InSequence s;

    EXPECT_CALL(f.callback, get_executor()).WillOnce(Return(ozo::tests::executor {&f.callback_executor}));
    EXPECT_CALL(f.strand, post(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(f.callback_executor, dispatch(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(f.callback, call(error_code{error::error}, f.conn))
        .WillOnce(Return());

    ozo::impl::make_async_connect_op(f.context)(error_code{error::error});

    EXPECT_EQ(f.conn->error_context_, "error while connection polling");
}

TEST_F(async_connect_op_call, should_preserve_not_empty_connection_error_context_on_error) {
    fixture f;
    f.conn->error_context_ = "my error";

    const InSequence s;

    EXPECT_CALL(f.callback, get_executor()).WillOnce(Return(ozo::tests::executor {&f.callback_executor}));
    EXPECT_CALL(f.strand, post(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(f.callback_executor, dispatch(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(f.callback, call(error_code{error::error}, f.conn))
        .WillOnce(Return());

    ozo::impl::make_async_connect_op(f.context)(error_code{error::error});

    EXPECT_EQ(f.conn->error_context_, "my error");
}

struct async_connect : Test {
    fixture f;
};

TEST_F(async_connect, should_cancel_timer_when_operation_is_done_before_timeout) {
    *f.conn->handle_ = native_handle::good;

    Sequence s;

    EXPECT_CALL(f.strand_service, get_executor()).InSequence(s).WillOnce(ReturnRef(f.strand));

    EXPECT_CALL(f.connection, start_connection("conninfo")).InSequence(s).WillOnce(Return(error_code{}));
    EXPECT_CALL(f.connection, assign_socket()).InSequence(s).WillOnce(Return(error_code{}));
    EXPECT_CALL(f.timer, expires_after(time_traits::duration(42))).InSequence(s).WillOnce(Return(0));
    EXPECT_CALL(f.timer, async_wait(_)).InSequence(s).WillOnce(Return());

    EXPECT_CALL(f.socket, async_write_some(_)).InSequence(s).WillOnce(InvokeArgument<0>(error_code{}));

    EXPECT_CALL(f.strand, post(_)).InSequence(s).WillOnce(InvokeArgument<0>());

    EXPECT_CALL(f.connection, connect_poll()).InSequence(s).WillOnce(Return(PGRES_POLLING_OK));

    EXPECT_CALL(f.strand, post(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(f.timer, cancel()).InSequence(s).WillOnce(Return(0));

    EXPECT_CALL(f.callback, get_executor()).InSequence(s).WillOnce(Return(ozo::tests::executor {&f.callback_executor}));
    EXPECT_CALL(f.executor, post(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(f.callback_executor, dispatch(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(f.callback, call(error_code{}, f.conn)).InSequence(s).WillOnce(Return());

    ozo::impl::async_connect("conninfo", time_traits::duration(42), f.conn, wrap(f.callback));
}

TEST_F(async_connect, should_request_oid_map_when_oid_map_is_not_empty) {
    auto conn = make_connection(f.connection, f.io, f.socket, f.timer, ozo::register_types<custom_type>());
    StrictMock<callback_gmock<decltype(conn)>> callback {};

    *conn->handle_ = native_handle::good;

    Sequence s;

    EXPECT_CALL(f.strand_service, get_executor()).InSequence(s).WillOnce(ReturnRef(f.strand));

    EXPECT_CALL(f.connection, start_connection("conninfo")).InSequence(s).WillOnce(Return(error_code{}));
    EXPECT_CALL(f.connection, assign_socket()).InSequence(s).WillOnce(Return(error_code{}));
    EXPECT_CALL(f.timer, expires_after(time_traits::duration(42))).InSequence(s).WillOnce(Return(0));
    EXPECT_CALL(f.timer, async_wait(_)).InSequence(s).WillOnce(Return());

    EXPECT_CALL(f.socket, async_write_some(_)).InSequence(s).WillOnce(InvokeArgument<0>(error_code{}));

    EXPECT_CALL(f.strand, post(_)).InSequence(s).WillOnce(InvokeArgument<0>());

    EXPECT_CALL(f.connection, connect_poll()).InSequence(s).WillOnce(Return(PGRES_POLLING_OK));

    EXPECT_CALL(f.strand, post(_)).InSequence(s).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(f.connection, async_request()).InSequence(s).WillOnce(Return());

    ozo::impl::async_connect("conninfo", time_traits::duration(42), conn, wrap(callback));
}

} // namespace
