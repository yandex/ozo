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
        auto transaction = ozo::begin(conn_info[io], yield);
        ASSERT_TRUE(transaction);
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
        auto transaction = ozo::begin(conn_info[io], yield);
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

TEST(transaction, transaction_level_options_should_not_cause_sql_syntax_errors) {
    ozo::io_context io;
    ozo::connection_info<> conn_info(OZO_PG_TEST_CONNINFO);

    asio::spawn(io, [&] (asio::yield_context yield) {
        auto level = ozo::isolation_level::serializable;
        auto const options = ozo::make_options(ozo::transaction_options::isolation_level = level);
        ozo::error_code ec;
        auto transaction = ozo::begin.with_transaction_options(options)(conn_info[io], ozo::none, yield[ec]);

        static_assert(std::is_same_v<decltype(ozo::get_transaction_isolation_level(transaction)), decltype(level)>);
        static_assert(ozo::is_none<decltype(ozo::get_transaction_mode(transaction))>::value);
        static_assert(ozo::is_none<decltype(ozo::get_transaction_deferrability(transaction))>::value);

        EXPECT_FALSE(ec);
        ozo::rollback(std::move(transaction), yield[ec]);
        EXPECT_FALSE(ec);
    });

    asio::spawn(io, [&] (asio::yield_context yield) {
        auto level = ozo::isolation_level::repeatable_read;
        auto const options = ozo::make_options(ozo::transaction_options::isolation_level = level);
        ozo::error_code ec;
        auto transaction = ozo::begin.with_transaction_options(options)(conn_info[io], ozo::none, yield[ec]);

        static_assert(std::is_same_v<decltype(ozo::get_transaction_isolation_level(transaction)), decltype(level)>);
        static_assert(ozo::is_none<decltype(ozo::get_transaction_mode(transaction))>::value);
        static_assert(ozo::is_none<decltype(ozo::get_transaction_deferrability(transaction))>::value);

        EXPECT_FALSE(ec);
        ozo::rollback(std::move(transaction), yield[ec]);
        EXPECT_FALSE(ec);
    });

    asio::spawn(io, [&] (asio::yield_context yield) {
        auto level = ozo::isolation_level::read_committed;
        auto const options = ozo::make_options(ozo::transaction_options::isolation_level = level);
        ozo::error_code ec;
        auto transaction = ozo::begin.with_transaction_options(options)(conn_info[io], ozo::none, yield[ec]);

        static_assert(std::is_same_v<decltype(ozo::get_transaction_isolation_level(transaction)), decltype(level)>);
        static_assert(ozo::is_none<decltype(ozo::get_transaction_mode(transaction))>::value);
        static_assert(ozo::is_none<decltype(ozo::get_transaction_deferrability(transaction))>::value);

        EXPECT_FALSE(ec);
        ozo::rollback(std::move(transaction), yield[ec]);
        EXPECT_FALSE(ec);
    });

    asio::spawn(io, [&] (asio::yield_context yield) {
        auto level = ozo::isolation_level::read_uncommitted;
        auto const options = ozo::make_options(ozo::transaction_options::isolation_level = level);
        ozo::error_code ec;
        auto transaction = ozo::begin.with_transaction_options(options)(conn_info[io], ozo::none, yield[ec]);

        static_assert(std::is_same_v<decltype(ozo::get_transaction_isolation_level(transaction)), decltype(level)>);
        static_assert(ozo::is_none<decltype(ozo::get_transaction_mode(transaction))>::value);
        static_assert(ozo::is_none<decltype(ozo::get_transaction_deferrability(transaction))>::value);

        EXPECT_FALSE(ec);
        ozo::rollback(std::move(transaction), yield[ec]);
        EXPECT_FALSE(ec);
    });

    io.run();
}

TEST(transaction, transaction_mode_options_should_not_cause_sql_syntax_errors) {
    ozo::io_context io;
    ozo::connection_info<> conn_info(OZO_PG_TEST_CONNINFO);

    asio::spawn(io, [&] (asio::yield_context yield) {
        auto mode = ozo::transaction_mode::read_write;
        auto const options = ozo::make_options(ozo::transaction_options::mode = mode);
        ozo::error_code ec;
        auto transaction = ozo::begin.with_transaction_options(options)(conn_info[io], ozo::none, yield[ec]);

        static_assert(ozo::is_none<decltype(ozo::get_transaction_isolation_level(transaction))>::value);
        static_assert(std::is_same_v<decltype(ozo::get_transaction_mode(transaction)), decltype(mode)>);
        static_assert(ozo::is_none<decltype(ozo::get_transaction_deferrability(transaction))>::value);

        EXPECT_FALSE(ec);
        ozo::rollback(std::move(transaction), yield[ec]);
        EXPECT_FALSE(ec);
    });

    asio::spawn(io, [&] (asio::yield_context yield) {
        auto mode = ozo::transaction_mode::read_only;
        auto const options = ozo::make_options(ozo::transaction_options::mode = mode);
        ozo::error_code ec;
        auto transaction = ozo::begin.with_transaction_options(options)(conn_info[io], ozo::none, yield[ec]);

        static_assert(ozo::is_none<decltype(ozo::get_transaction_isolation_level(transaction))>::value);
        static_assert(std::is_same_v<decltype(ozo::get_transaction_mode(transaction)), decltype(mode)>);
        static_assert(ozo::is_none<decltype(ozo::get_transaction_deferrability(transaction))>::value);

        EXPECT_FALSE(ec);
        ozo::rollback(std::move(transaction), yield[ec]);
        EXPECT_FALSE(ec);
    });

    io.run();
}

TEST(transaction, transaction_deferrability_options_should_not_generate_syntax_errors) {
    using ozo::transaction_options;
    ozo::io_context io;
    ozo::connection_info<> conn_info(OZO_PG_TEST_CONNINFO);

    asio::spawn(io, [&] (asio::yield_context yield) {
        auto defer = ozo::deferrable;
        auto const options = ozo::make_options(ozo::transaction_options::deferrability = defer);
        ozo::error_code ec;
        auto transaction = ozo::begin.with_transaction_options(options)(conn_info[io], yield[ec]);

        static_assert(ozo::is_none<decltype(ozo::get_transaction_isolation_level(transaction))>::value);
        static_assert(ozo::is_none<decltype(ozo::get_transaction_mode(transaction))>::value);
        static_assert(std::is_same_v<decltype(ozo::get_transaction_deferrability(transaction)), decltype(defer)>);

        EXPECT_FALSE(ec);
        ozo::rollback(std::move(transaction), yield[ec]);
        EXPECT_FALSE(ec);
    });

    asio::spawn(io, [&] (asio::yield_context yield) {
        auto defer = !ozo::deferrable;
        auto const options = ozo::make_options(ozo::transaction_options::deferrability = defer);
        ozo::error_code ec;
        auto transaction = ozo::begin.with_transaction_options(options)(conn_info[io], yield[ec]);

        static_assert(ozo::is_none<decltype(ozo::get_transaction_isolation_level(transaction))>::value);
        static_assert(ozo::is_none<decltype(ozo::get_transaction_mode(transaction))>::value);
        static_assert(std::is_same_v<decltype(ozo::get_transaction_deferrability(transaction)), decltype(defer)>);

        EXPECT_FALSE(ec);
        ozo::rollback(std::move(transaction), yield[ec]);
        EXPECT_FALSE(ec);
    });

    io.run();
}

} // namespace
