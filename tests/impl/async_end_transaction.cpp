#include "connection_mock.h"

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
    StrictMock<executor_gmock> callback_executor{};
    StrictMock<callback_gmock<connection_ptr<>>> callback {};
    StrictMock<executor_gmock> executor {};
    StrictMock<executor_gmock> strand {};
    StrictMock<strand_executor_service_gmock> strand_service {};
    StrictMock<stream_descriptor_gmock> socket {};
    StrictMock<steady_timer_gmock> timer {};
    io_context io {executor, strand_service};
    decltype(make_connection(connection, io, socket, timer)) conn = make_connection(connection, io, socket, timer);
    time_traits::duration timeout {42};
};

TEST_F(async_end_transaction, should_call_async_execute) {
    *conn->handle_ = native_handle::good;

    auto transaction = ozo::impl::transaction<decltype(conn)>(std::move(conn));

    const InSequence s;

    EXPECT_CALL(connection, async_execute()).WillOnce(Return());

    ozo::impl::async_end_transaction(std::move(transaction), fake_query {}, timeout, wrap(callback));
}

} // namespace
