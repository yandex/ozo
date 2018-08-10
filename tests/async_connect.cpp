#include "connection_mock.h"
#include "test_error.h"

#include <ozo/impl/async_connect.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace {

namespace hana = boost::hana;

using namespace testing;
using namespace ozo::tests;

using ozo::empty_oid_map;

template <typename OidMap>
struct fixture_impl {
    StrictMock<connection_gmock> connection{};
    StrictMock<executor_gmock> executor{};
    StrictMock<executor_gmock> strand{};
    StrictMock<strand_executor_service_gmock> strand_service{};
    StrictMock<stream_descriptor_gmock> socket{};
    io_context io{executor, strand_service};
    decltype(make_connection(connection, io, socket)) conn =
            make_connection(connection, io, socket);
    StrictMock<callback_gmock<decltype(conn)>> callback{};
    StrictMock<steady_timer_gmock> timer;
    steady_timer timer_wrapper {timer};
    decltype(ozo::impl::make_connect_operation_context(conn, wrap(callback), timer_wrapper)) context;

    fixture_impl() : context(make_operation_context()) {}

    auto make_operation_context() {
        EXPECT_CALL(strand_service, get_executor()).WillOnce(ReturnRef(strand));
        return ozo::impl::make_connect_operation_context(conn, wrap(callback), timer_wrapper);
    }
};

using fixture = fixture_impl<empty_oid_map>;

template <typename OidMap>
auto make_fixture(OidMap) { return fixture_impl<OidMap>{}; }

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
    EXPECT_CALL(f.timer, cancel()).WillOnce(Return(0));

    EXPECT_CALL(f.executor, post(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(f.callback, context_preserved()).WillOnce(Return());
    EXPECT_CALL(f.callback, call(error_code{ozo::error::pq_connection_start_failed}, f.conn))
        .WillOnce(Return());

    ozo::impl::make_async_connect_op(f.context).perform("conninfo", time_traits::duration(42));
}

TEST_F(async_connect_op, should_call_handler_with_pq_connection_status_bad_if_connection_status_is_bad) {
    fixture f;
    *(f.conn->handle_) = native_handle::bad;

    const InSequence s;

    EXPECT_CALL(f.connection, start_connection("conninfo")).WillOnce(Return(error_code{}));
    EXPECT_CALL(f.timer, cancel()).WillOnce(Return(0));

    EXPECT_CALL(f.executor, post(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(f.callback, context_preserved()).WillOnce(Return());
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
    EXPECT_CALL(f.timer, cancel()).WillOnce(Return(0));

    EXPECT_CALL(f.executor, post(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(f.callback, context_preserved()).WillOnce(Return());
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

    EXPECT_CALL(f.strand, dispatch(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(f.callback, context_preserved()).WillOnce(Return());
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

    EXPECT_CALL(f.strand, dispatch(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(f.callback, context_preserved()).WillOnce(Return());

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

    EXPECT_CALL(f.strand, dispatch(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(f.callback, context_preserved()).WillOnce(Return());

    EXPECT_CALL(f.connection, connect_poll()).WillOnce(Return(PGRES_POLLING_OK));
    EXPECT_CALL(f.timer, cancel()).WillOnce(Return(1));

    EXPECT_CALL(f.executor, post(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(f.callback, context_preserved()).WillOnce(Return());
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

    EXPECT_CALL(f.strand, dispatch(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(f.callback, context_preserved()).WillOnce(Return());
    EXPECT_CALL(f.connection, connect_poll()).WillOnce(Return(PGRES_POLLING_FAILED));
    EXPECT_CALL(f.timer, cancel()).WillOnce(Return(1));

    EXPECT_CALL(f.executor, post(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(f.callback, context_preserved()).WillOnce(Return());
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

    EXPECT_CALL(f.strand, dispatch(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(f.callback, context_preserved()).WillOnce(Return());

    EXPECT_CALL(f.connection, connect_poll()).WillOnce(Return(PGRES_POLLING_ACTIVE));
    EXPECT_CALL(f.timer, cancel()).WillOnce(Return(1));

    EXPECT_CALL(f.executor, post(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(f.callback, context_preserved()).WillOnce(Return());
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

    EXPECT_CALL(f.strand, dispatch(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(f.callback, context_preserved()).WillOnce(Return());
    EXPECT_CALL(f.timer, cancel()).WillOnce(Return(1));

    EXPECT_CALL(f.executor, post(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(f.callback, context_preserved()).WillOnce(Return());
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

    EXPECT_CALL(f.strand, dispatch(_)).WillOnce(InvokeArgument<0>());
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

    EXPECT_CALL(f.strand, dispatch(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(f.callback, context_preserved()).WillOnce(Return());

    EXPECT_CALL(f.connection, connect_poll()).WillOnce(Return(PGRES_POLLING_OK));
    EXPECT_CALL(f.timer, cancel()).WillOnce(Return(1));

    EXPECT_CALL(f.executor, post(_)).WillOnce(InvokeArgument<0>());
    EXPECT_CALL(f.callback, context_preserved()).WillOnce(Return());
    EXPECT_CALL(f.callback, call(error_code{}, f.conn)).WillOnce(Return());

    ozo::impl::make_async_connect_op(f.context).perform("conninfo", time_traits::duration(42));

    EXPECT_CALL(f.strand, dispatch(_)).WillOnce(InvokeArgument<0>());

    on_timeout(error_code {boost::asio::error::operation_aborted});
}

} // namespace
