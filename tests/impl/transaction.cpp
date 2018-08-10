#include "connection_mock.h"

#include <ozo/impl/transaction.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace {

using namespace testing;
using namespace ozo::tests;

struct impl_transaction : Test {
    StrictMock<connection_gmock> connection {};
    StrictMock<executor_gmock> executor {};
    StrictMock<strand_executor_service_gmock> strand_service {};
    StrictMock<stream_descriptor_gmock> socket {};
    StrictMock<steady_timer_gmock> timer {};
    io_context io {executor, strand_service};
    decltype(make_connection(connection, io, socket, timer)) conn = make_connection(connection, io, socket, timer);
};

TEST_F(impl_transaction, should_be_able_to_construct_default) {
    ozo::impl::transaction<decltype(conn)> t;
}

TEST_F(impl_transaction, when_destruct_last_copy_with_connection_should_close_connection) {
    EXPECT_CALL(socket, close(_)).WillOnce(Return());

    ozo::impl::make_transaction(std::move(conn));
}

TEST_F(impl_transaction, when_destruct_last_copy_without_connection_should_not_close_connection) {
    EXPECT_CALL(socket, close(_)).Times(0);

    ozo::impl::make_transaction(std::move(conn)).take_connection(conn);
}

} // namespace
