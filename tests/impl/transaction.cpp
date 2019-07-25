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
    io_context io {executor, strand_service};
    decltype(make_connection(connection, io, socket)) conn = make_connection(connection, io, socket);
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

TEST_F(impl_transaction, should_be_able_to_convert_to_bool) {
    ozo::impl::transaction<decltype(conn)> t;
    EXPECT_FALSE(static_cast<bool>(t));
}

TEST_F(impl_transaction, has_connection_when_constructed_with) {
    EXPECT_CALL(socket, close(_)).WillOnce(Return());

    EXPECT_TRUE(ozo::impl::make_transaction(std::move(conn)).has_connection());
}

TEST_F(impl_transaction, transaction_with_initialized_connection_is_not_null) {
    EXPECT_CALL(socket, close(_)).WillOnce(Return());

    EXPECT_FALSE(is_null(ozo::impl::make_transaction(std::move(conn))));
}

TEST_F(impl_transaction, transaction_without_connection_is_null) {
    ozo::impl::transaction<decltype(conn)> transaction;
    EXPECT_TRUE(is_null(transaction));
}

TEST_F(impl_transaction, transaction_without_null_state_connection_is_null) {
    ozo::impl::transaction<decltype(conn)> transaction(nullptr);
    EXPECT_TRUE(is_null(transaction));
}

TEST_F(impl_transaction, transaction_become_null_after_take_connection) {
    auto transaction = ozo::impl::make_transaction(std::move(conn));

    transaction.take_connection(conn);

    EXPECT_TRUE(is_null(transaction));
}

} // namespace
