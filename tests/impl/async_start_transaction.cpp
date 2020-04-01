#include "connection_mock.h"

#include <ozo/core/options.h>
#include <ozo/impl/async_start_transaction.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace {

using namespace testing;
using namespace ozo::tests;

using ozo::error_code;
using ozo::time_traits;

struct async_start_transaction : Test {
    decltype(ozo::make_options()) options = ozo::make_options();
    StrictMock<connection_gmock> connection {};
    StrictMock<executor_mock> callback_executor{};
    StrictMock<callback_gmock<ozo::impl::transaction<connection_ptr<>, decltype(options)>>> callback {};
    StrictMock<executor_mock> strand {};
    io_context io;
    StrictMock<PGconn_mock> handle;
    connection_ptr<> conn = make_connection(connection, io, handle);
    time_traits::duration timeout {42};
};

TEST_F(async_start_transaction, should_call_async_execute) {
    EXPECT_CALL(handle, PQstatus()).WillRepeatedly(Return(CONNECTION_OK));

    EXPECT_CALL(connection, async_execute()).WillOnce(Return());

    ozo::impl::async_start_transaction(conn, options, fake_query {}, timeout, wrap(callback));
}

} // namespace
