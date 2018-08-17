#include <ozo/connection_info.h>
#include <ozo/connection_pool.h>
#include <ozo/query_builder.h>
#include <ozo/request.h>

#include <boost/asio/spawn.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace ozo::tests {
struct custom_type {};
} // namespace ozo::tests

OZO_PG_DEFINE_CUSTOM_TYPE(ozo::tests::custom_type, "custom_type", dynamic_size)

namespace {

namespace hana = boost::hana;

using namespace testing;
using namespace ozo::tests;

template <typename ...Ts>
using rows_of = std::vector<std::tuple<Ts...>>;

TEST(request, should_return_error_and_bad_connect_for_invalid_connection_info) {
    using namespace ozo::literals;
    using namespace hana::literals;

    ozo::io_context io;
    ozo::connection_info<> conn_info("invalid connection info");

    ozo::result res;
    ozo::request(ozo::make_connector(conn_info, io), "SELECT 1"_SQL + " + 1"_SQL, std::ref(res),
            [](ozo::error_code ec, auto conn) {
        EXPECT_TRUE(ec);
        EXPECT_TRUE(ozo::connection_bad(conn));
    });

    io.run();
}

TEST(request, should_return_selected_variable) {
    using namespace ozo::literals;
    using namespace hana::literals;

    ozo::io_context io;
    ozo::connection_info<> conn_info(OZO_PG_TEST_CONNINFO);

    ozo::result res;
    const std::string foo = "foo";
    ozo::request(ozo::make_connector(conn_info, io), "SELECT "_SQL + foo, std::ref(res),
            [&](ozo::error_code ec, auto conn) {
        ASSERT_FALSE(ec) << ec.message() << " | " << error_message(conn) << " | " << get_error_context(conn);
        ASSERT_EQ(1u, res.size());
        ASSERT_EQ(1u, res[0].size());
        EXPECT_EQ(std::string("foo"), res[0][0].data());
        EXPECT_FALSE(ozo::connection_bad(conn));
    });

    io.run();
}

TEST(request, should_return_selected_string_array) {
    using namespace ozo::literals;
    using namespace hana::literals;

    ozo::io_context io;
    ozo::connection_info<> conn_info(OZO_PG_TEST_CONNINFO);

    const std::vector<std::string> foos = {"foo", "buzz", "bar"};

    rows_of<std::vector<std::string>> res;
    ozo::request(ozo::make_connector(conn_info, io), "SELECT "_SQL + foos, std::back_inserter(res),
            [&](ozo::error_code ec, auto conn) {
        ASSERT_FALSE(ec) << ec.message() << " | " << error_message(conn) << " | " << get_error_context(conn);
        ASSERT_EQ(1u, res.size());
        EXPECT_THAT(std::get<0>(res[0]), ElementsAre("foo", "buzz", "bar"));
        EXPECT_FALSE(ozo::connection_bad(conn));
    });

    io.run();
}

TEST(request, should_return_selected_int_array) {
    using namespace ozo::literals;
    using namespace hana::literals;

    ozo::io_context io;
    ozo::connection_info<> conn_info(OZO_PG_TEST_CONNINFO);

    const std::vector<int32_t> foos = {1, 22, 333};

    rows_of<std::vector<int32_t>> res;
    ozo::request(ozo::make_connector(conn_info, io), "SELECT "_SQL + foos, std::back_inserter(res),
            [&](ozo::error_code ec, auto conn) {
        ASSERT_FALSE(ec) << ec.message() << " | " << error_message(conn) << " | " << get_error_context(conn);
        ASSERT_EQ(1u, res.size());
        EXPECT_THAT(std::get<0>(res[0]), ElementsAre(1, 22, 333));
        EXPECT_FALSE(ozo::connection_bad(conn));
    });

    io.run();
}

TEST(request, should_fill_oid_map_when_oid_map_is_not_empty) {
    using namespace ozo::literals;
    namespace asio = boost::asio;

    ozo::io_context io;
    constexpr const auto oid_map = ozo::register_types<custom_type>();
    ozo::connection_info<> conn_info(OZO_PG_TEST_CONNINFO);
    ozo::connection_info<std::decay_t<decltype(oid_map)>> conn_info_with_oid_map(OZO_PG_TEST_CONNINFO);

    asio::spawn(io, [&] (asio::yield_context yield) {
        ozo::result result;
        auto conn = ozo::request(ozo::make_connector(conn_info, io), "DROP TYPE IF EXISTS custom_type"_SQL, std::ref(result), yield);
        ozo::request(conn, "CREATE TYPE custom_type AS ()"_SQL, std::ref(result), yield);
        auto conn_with_oid_map = ozo::get_connection(ozo::make_connector(conn_info_with_oid_map, io), yield);
        EXPECT_NE(ozo::type_oid<custom_type>(ozo::get_oid_map(conn_with_oid_map)), ozo::null_oid);
    });

    io.run();
}

TEST(request, should_request_with_connection_pool) {
    using namespace ozo::literals;
    namespace asio = boost::asio;

    ozo::io_context io;
    ozo::connection_info<> conn_info(OZO_PG_TEST_CONNINFO);
    const ozo::connection_pool_config config;
    auto pool = ozo::make_connection_pool(conn_info, config);
    asio::spawn(io, [&] (asio::yield_context yield) {
        ozo::result result;
        ozo::request(ozo::make_connector(pool, io), "SELECT 1"_SQL, std::ref(result), yield);
    });

    io.run();
}

TEST(request, should_call_handler_with_error_for_zero_timeout) {
    using namespace ozo::literals;
    namespace asio = boost::asio;

    ozo::io_context io;
    const ozo::connection_info<> conn_info(OZO_PG_TEST_CONNINFO);
    const ozo::time_traits::duration timeout(0);

    ozo::result res;
    std::atomic_flag called {};
    ozo::request(ozo::make_connector(conn_info, io), "SELECT 1"_SQL, timeout, std::ref(res),
            [&](ozo::error_code ec, auto conn) {
        EXPECT_FALSE(called.test_and_set());
        EXPECT_EQ(ec, boost::system::error_condition(boost::system::errc::operation_canceled));
        EXPECT_FALSE(ozo::connection_bad(conn));
        EXPECT_EQ(ozo::get_error_context(conn), "error while get request result");
    });

    io.run();
}

TEST(request, should_return_result_for_max_timeout) {
    using namespace ozo::literals;
    namespace asio = boost::asio;

    ozo::io_context io;
    const ozo::connection_info<> conn_info(OZO_PG_TEST_CONNINFO);
    const auto timeout = ozo::time_traits::duration::max();

    rows_of<std::int32_t> res;
    std::atomic_flag called {};
    ozo::request(ozo::make_connector(conn_info, io), "SELECT 1"_SQL, timeout, std::back_inserter(res),
            [&](ozo::error_code ec, auto conn) {
        EXPECT_FALSE(called.test_and_set());
        ASSERT_FALSE(ec) << ec.message() << " | " << error_message(conn) << " | " << get_error_context(conn);
        EXPECT_THAT(res, ElementsAre(std::make_tuple(1)));
        EXPECT_FALSE(ozo::connection_bad(conn));
    });

    io.run();
}

} // namespace
