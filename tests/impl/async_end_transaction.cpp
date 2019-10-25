#include "connection_mock.h"

#include <ozo/core/options.h>
#include <ozo/impl/async_end_transaction.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace {

using namespace testing;
using namespace ozo::tests;

using ozo::error_code;
using ozo::time_traits;

struct async_end_transaction : Test {
    StrictMock<connection_gmock> connection {};
    StrictMock<executor_mock> callback_executor{};
    StrictMock<callback_gmock<connection_ptr<>>> callback {};
    StrictMock<executor_mock> strand {};
    StrictMock<stream_descriptor_mock> socket {};
    io_context io;
    decltype(make_connection(connection, io, socket)) conn = make_connection(connection, io, socket);
    decltype(ozo::make_options()) options = ozo::make_options();
    time_traits::duration timeout {42};
};

TEST_F(async_end_transaction, should_call_async_execute) {
    *conn->handle_ = native_handle::good;

    auto transaction = ozo::impl::transaction<decltype(conn), decltype(options)>(std::move(conn), options);

    const InSequence s;

    EXPECT_CALL(connection, async_execute()).WillOnce(Return());

    ozo::impl::async_end_transaction(std::move(transaction), fake_query {}, timeout, wrap(callback));
}

} // namespace
