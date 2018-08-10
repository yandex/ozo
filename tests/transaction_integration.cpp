#include <ozo/connection_info.h>
#include <ozo/query_builder.h>
#include <ozo/result.h>
#include <ozo/request.h>
#include <ozo/transaction.h>

#include <boost/asio/spawn.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace {

namespace asio = boost::asio;

using namespace testing;

TEST(transaction, create_schema_in_transaction_and_commit_then_table_should_exist) {
    using namespace ozo::literals;

    ozo::io_context io;
    ozo::connection_info<> conn_info(OZO_PG_TEST_CONNINFO);

    asio::spawn(io, [&] (asio::yield_context yield) {
        auto transaction = ozo::begin(ozo::make_connector(conn_info, io), yield);
        ozo::result result;
        ozo::request(transaction, "DROP SCHEMA IF EXISTS ozo_test CASCADE;"_SQL, std::ref(result), yield);
        ozo::request(transaction, "CREATE SCHEMA ozo_test;"_SQL, std::ref(result), yield);
        auto connection = ozo::commit(std::move(transaction), yield);
        ozo::request(connection, "DROP SCHEMA ozo_test;"_SQL, std::ref(result), yield);
    });

    io.run();
}

TEST(transaction, create_schema_in_transaction_and_rollback_then_table_should_not_exist) {
    using namespace ozo::literals;

    ozo::io_context io;
    ozo::connection_info<> conn_info(OZO_PG_TEST_CONNINFO);

    asio::spawn(io, [&] (asio::yield_context yield) {
        auto transaction = ozo::begin(ozo::make_connector(conn_info, io), yield);
        ozo::result result;
        ozo::request(transaction, "DROP SCHEMA IF EXISTS ozo_test CASCADE;"_SQL, std::ref(result), yield);
        ozo::request(transaction, "CREATE SCHEMA ozo_test;"_SQL, std::ref(result), yield);
        auto connection = ozo::rollback(std::move(transaction), yield);
        ozo::error_code ec;
        ozo::request(connection, "DROP SCHEMA ozo_test;"_SQL, std::ref(result), yield[ec]);
        EXPECT_EQ(ec, ozo::error_condition(ozo::sqlstate::invalid_schema_name));
    });

    io.run();
}

} // namespace
