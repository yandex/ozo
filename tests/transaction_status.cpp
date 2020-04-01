#include <ozo/transaction_status.h>
#include "connection_mock.h"
#include <boost/asio/spawn.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace {

using namespace testing;

struct get_transaction_status : Test {
    ozo::tests::io_context io;
    StrictMock<ozo::tests::PGconn_mock> handle;
    auto make_connection() {
        using namespace ozo::tests;
        EXPECT_CALL(handle, PQstatus()).WillRepeatedly(Return(CONNECTION_OK));
        return std::make_shared<connection<>>(connection<>{
            std::addressof(handle),
            {}, nullptr, "", nullptr
        });
    }
};

TEST_F(get_transaction_status, should_return_transaction_status_unknown_for_null_transaction) {
    const auto conn = ozo::tests::connection_ptr<>();
    EXPECT_EQ(ozo::transaction_status::unknown, ozo::get_transaction_status(conn));
}

TEST_F(get_transaction_status, should_return_throw_for_unsupported_status) {
    const auto conn = make_connection();
    EXPECT_CALL(handle, PQtransactionStatus())
        .WillOnce(Return(static_cast<PGTransactionStatusType>(-1)));
    EXPECT_THROW(ozo::get_transaction_status(conn), std::invalid_argument);
}

namespace with_params {

struct get_transaction_status : ::get_transaction_status,
        WithParamInterface<std::tuple<PGTransactionStatusType, ozo::transaction_status>> {
};

TEST_P(get_transaction_status, should_return_status_for_connection){
    const auto conn = make_connection();
    EXPECT_CALL(handle, PQtransactionStatus())
        .WillOnce(Return(std::get<0>(GetParam())));
    EXPECT_EQ(std::get<1>(GetParam()), ozo::get_transaction_status(conn));
}

INSTANTIATE_TEST_SUITE_P(
    with_any_PGTransactionStatusType,
    get_transaction_status,
    testing::Values(
        std::make_tuple(PQTRANS_UNKNOWN, ozo::transaction_status::unknown),
        std::make_tuple(PQTRANS_IDLE, ozo::transaction_status::idle),
        std::make_tuple(PQTRANS_ACTIVE, ozo::transaction_status::active),
        std::make_tuple(PQTRANS_INTRANS, ozo::transaction_status::transaction),
        std::make_tuple(PQTRANS_INERROR, ozo::transaction_status::error)
    )
);

} // namespace with_params

}
