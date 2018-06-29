#include <ozo/impl/async_connect.h>
#include "test_error.h"
#include "connection_mock.h"
#include <GUnit/GTest.h>

namespace {

namespace hana = ::boost::hana;
using ozo::empty_oid_map;
using ozo::testing::native_handle;
using callback_mock = ozo::testing::callback_gmock<>;
using ozo::testing::wrap;
using ::testing::StrictMock;

template <typename OidMap>
struct fixture_impl {
    StrictMock<ozo::testing::connection_gmock> connection{};
    StrictMock<callback_mock> callback{};
    StrictMock<ozo::testing::executor_gmock> io_context{};
    StrictMock<ozo::testing::executor_gmock> strand{};
    StrictMock<ozo::testing::strand_executor_service_gmock> strand_service{};
    StrictMock<ozo::testing::stream_descriptor_gmock> socket{};
    ozo::testing::io_context io{io_context, strand_service};
    decltype(ozo::testing::make_connection(connection, io, socket)) conn =
            ozo::testing::make_connection(connection, io, socket);
};

using fixture = fixture_impl<empty_oid_map>;

template <typename OidMap>
auto make_fixture(OidMap) { return fixture_impl<OidMap>{}; }

GTEST("ozo::async_connect()") {
    using namespace testing;
    using ozo::error_code;

    SHOULD("start connection, assign socket and wait for write complete") {
        fixture f;
        *(f.conn->handle_) = native_handle::good;

        EXPECT_CALL(f.connection, start_connection("conninfo")).WillOnce(Return(error_code{}));
        EXPECT_CALL(f.connection, assign_socket()).WillOnce(Return(error_code{}));
        EXPECT_CALL(f.socket, async_write_some(_)).WillOnce(Return());
        ozo::impl::make_async_connect_op(f.conn, wrap(f.callback)).perform("conninfo");
    }

    SHOULD("call handler with pq_connection_start_failed on error in start_connection") {
        fixture f;
        *(f.conn->handle_) = native_handle::good;

        EXPECT_CALL(f.connection, start_connection("conninfo"))
            .WillOnce(Return(error_code{ozo::error::pq_connection_start_failed}));

        EXPECT_CALL(f.io_context, post(_)).WillOnce(InvokeArgument<0>());
        EXPECT_CALL(f.callback, context_preserved()).WillOnce(Return());
        EXPECT_CALL(f.callback, call(error_code{ozo::error::pq_connection_start_failed}))
            .WillOnce(Return());

        ozo::impl::make_async_connect_op(f.conn, wrap(f.callback)).perform("conninfo");
    }

    SHOULD("call handler with pq_connection_status_bad if connection status is bad") {
        fixture f;
        *(f.conn->handle_) = native_handle::bad;

        EXPECT_CALL(f.connection, start_connection("conninfo")).WillOnce(Return(error_code{}));

        EXPECT_CALL(f.io_context, post(_)).WillOnce(InvokeArgument<0>());
        EXPECT_CALL(f.callback, context_preserved()).WillOnce(Return());
        EXPECT_CALL(f.callback, call(error_code{ozo::error::pq_connection_status_bad}))
            .WillOnce(Return());

        ozo::impl::make_async_connect_op(f.conn, wrap(f.callback)).perform("conninfo");
    }

    SHOULD("call handler with error if assign_socket returns error") {
        fixture f;
        *(f.conn->handle_) = native_handle::good;

        EXPECT_CALL(f.connection, start_connection("conninfo")).WillOnce(Return(error_code{}));
        EXPECT_CALL(f.connection, assign_socket()).WillOnce(Return(error_code{ozo::testing::error::error}));

        EXPECT_CALL(f.io_context, post(_)).WillOnce(InvokeArgument<0>());
        EXPECT_CALL(f.callback, context_preserved()).WillOnce(Return());
        EXPECT_CALL(f.callback, call(error_code{ozo::testing::error::error})).WillOnce(Return());

        ozo::impl::make_async_connect_op(f.conn, wrap(f.callback)).perform("conninfo");
    }

    SHOULD("wait for write complete if connect_poll() returns PGRES_POLLING_WRITING") {
        fixture f;
        *(f.conn->handle_) = native_handle::good;

        testing::InSequence s;
        EXPECT_CALL(f.connection, start_connection("conninfo")).WillOnce(Return(error_code{}));
        EXPECT_CALL(f.connection, assign_socket()).WillOnce(Return(error_code{}));

        EXPECT_CALL(f.socket, async_write_some(_)).WillOnce(InvokeArgument<0>(error_code{}));
        EXPECT_CALL(f.callback, context_preserved()).WillOnce(Return());

        EXPECT_CALL(f.connection, connect_poll()).WillOnce(Return(PGRES_POLLING_WRITING));

        EXPECT_CALL(f.socket, async_write_some(_)).WillOnce(Return());
        ozo::impl::make_async_connect_op(f.conn, wrap(f.callback)).perform("conninfo");
    }

    SHOULD("wait for read complete if connect_poll() returns PGRES_POLLING_READING") {
        fixture f;
        *(f.conn->handle_) = native_handle::good;

        testing::InSequence s;
        EXPECT_CALL(f.connection, start_connection("conninfo")).WillOnce(Return(error_code{}));
        EXPECT_CALL(f.connection, assign_socket()).WillOnce(Return(error_code{}));

        EXPECT_CALL(f.socket, async_write_some(_)).WillOnce(InvokeArgument<0>(error_code{}));
        EXPECT_CALL(f.callback, context_preserved()).WillOnce(Return());

        EXPECT_CALL(f.connection, connect_poll()).WillOnce(Return(PGRES_POLLING_READING));

        EXPECT_CALL(f.socket, async_read_some(_)).WillOnce(Return());
        ozo::impl::make_async_connect_op(f.conn, wrap(f.callback)).perform("conninfo");
    }

    SHOULD("call handler with no error if connect_poll() returns PGRES_POLLING_OK") {
        fixture f;
        *(f.conn->handle_) = native_handle::good;

        testing::InSequence s;
        EXPECT_CALL(f.connection, start_connection("conninfo")).WillOnce(Return(error_code{}));
        EXPECT_CALL(f.connection, assign_socket()).WillOnce(Return(error_code{}));

        EXPECT_CALL(f.socket, async_write_some(_)).WillOnce(InvokeArgument<0>(error_code{}));
        EXPECT_CALL(f.callback, context_preserved()).WillOnce(Return());

        EXPECT_CALL(f.connection, connect_poll()).WillOnce(Return(PGRES_POLLING_OK));

        EXPECT_CALL(f.io_context, post(_)).WillOnce(InvokeArgument<0>());
        EXPECT_CALL(f.callback, context_preserved()).WillOnce(Return());
        EXPECT_CALL(f.callback, call(error_code{})).WillOnce(Return());
        ozo::impl::make_async_connect_op(f.conn, wrap(f.callback)).perform("conninfo");
    }

    SHOULD("call handler with pq_connect_poll_failed if connect_poll() returns PGRES_POLLING_FAILED") {
        fixture f;
        *(f.conn->handle_) = native_handle::good;

        testing::InSequence s;
        EXPECT_CALL(f.connection, start_connection("conninfo")).WillOnce(Return(error_code{}));
        EXPECT_CALL(f.connection, assign_socket()).WillOnce(Return(error_code{}));

        EXPECT_CALL(f.socket, async_write_some(_)).WillOnce(InvokeArgument<0>(error_code{}));
        EXPECT_CALL(f.callback, context_preserved()).WillOnce(Return());

        EXPECT_CALL(f.connection, connect_poll()).WillOnce(Return(PGRES_POLLING_FAILED));

        EXPECT_CALL(f.io_context, post(_)).WillOnce(InvokeArgument<0>());
        EXPECT_CALL(f.callback, context_preserved()).WillOnce(Return());
        EXPECT_CALL(f.callback, call(error_code{ozo::error::pq_connect_poll_failed}))
            .WillOnce(Return());
        ozo::impl::make_async_connect_op(f.conn, wrap(f.callback)).perform("conninfo");
    }

    SHOULD("call handler with pq_connect_poll_failed if connect_poll() returns PGRES_POLLING_ACTIVE") {
        fixture f;
        *(f.conn->handle_) = native_handle::good;

        testing::InSequence s;
        EXPECT_CALL(f.connection, start_connection("conninfo")).WillOnce(Return(error_code{}));
        EXPECT_CALL(f.connection, assign_socket()).WillOnce(Return(error_code{}));

        EXPECT_CALL(f.socket, async_write_some(_)).WillOnce(InvokeArgument<0>(error_code{}));
        EXPECT_CALL(f.callback, context_preserved()).WillOnce(Return());

        EXPECT_CALL(f.connection, connect_poll()).WillOnce(Return(PGRES_POLLING_ACTIVE));

        EXPECT_CALL(f.io_context, post(_)).WillOnce(InvokeArgument<0>());
        EXPECT_CALL(f.callback, context_preserved()).WillOnce(Return());
        EXPECT_CALL(f.callback, call(error_code{ozo::error::pq_connect_poll_failed}))
            .WillOnce(Return());
        ozo::impl::make_async_connect_op(f.conn, wrap(f.callback)).perform("conninfo");
    }

    SHOULD("call handler with the error if polling operation invokes callback with it") {
        fixture f;
        *(f.conn->handle_) = native_handle::good;

        testing::InSequence s;
        EXPECT_CALL(f.connection, start_connection("conninfo")).WillOnce(Return(error_code{}));
        EXPECT_CALL(f.connection, assign_socket()).WillOnce(Return(error_code{}));

        EXPECT_CALL(f.socket, async_write_some(_))
            .WillOnce(InvokeArgument<0>(ozo::testing::error::error));
        EXPECT_CALL(f.callback, context_preserved()).WillOnce(Return());

        EXPECT_CALL(f.io_context, post(_)).WillOnce(InvokeArgument<0>());
        EXPECT_CALL(f.callback, context_preserved()).WillOnce(Return());
        EXPECT_CALL(f.callback, call(error_code{ozo::testing::error::error}))
            .WillOnce(Return());
        ozo::impl::make_async_connect_op(f.conn, wrap(f.callback)).perform("conninfo");
    }
}

} // namespace
