#include <GUnit/GTest.h>

#include <ozo/async_request.h>
#include <ozo/connection_info.h>
#include <ozo/query_builder.h>
#include <ozo/request.h>

#include <boost/asio/spawn.hpp>

namespace ozo::testing {
struct custom_type {};
} // namespace ozo::testing

OZO_PG_DEFINE_CUSTOM_TYPE(ozo::testing::custom_type, "custom_type", dynamic_size)

template <typename ...Ts>
using rows_of = std::vector<std::tuple<Ts...>>;

GTEST("ozo::request") {
    using namespace ::testing;
    SHOULD("return error and bad connect for invalid connection info") {
        using namespace ozo::literals;
        using namespace boost::hana::literals;

        ozo::io_context io;
        ozo::connection_info<> conn_info(io, "invalid connection info");

        ozo::result res;
        ozo::async_request(conn_info, "SELECT 1"_SQL + " + 1"_SQL, res,
                [](ozo::error_code ec, auto conn) {
            EXPECT_TRUE(ec);
            EXPECT_TRUE(ozo::connection_bad(conn));
        });

        io.run();
    }

    SHOULD("return selected variable") {
        using namespace ozo::literals;
        using namespace boost::hana::literals;

        ozo::io_context io;
        ozo::connection_info<> conn_info(io, OZO_PG_TEST_CONNINFO);

        ozo::result res;
        const std::string foo = "foo";
        ozo::async_request(conn_info, "SELECT "_SQL + foo, res,
                [&](ozo::error_code ec, auto conn) {
            ASSERT_FALSE(ec) << ec.message() << " | " << error_message(conn) << " | " << get_error_context(conn);
            ASSERT_EQ(1, res.size());
            ASSERT_EQ(1, res[0].size());
            EXPECT_EQ(std::string("foo"), res[0][0].data());
            EXPECT_FALSE(ozo::connection_bad(conn));
        });

        io.run();
    }

    SHOULD("return selected string array") {
        using namespace ozo::literals;
        using namespace boost::hana::literals;

        ozo::io_context io;
        ozo::connection_info<> conn_info(io, OZO_PG_TEST_CONNINFO);

        const std::vector<std::string> foos = {"foo", "buzz", "bar"};

        rows_of<std::vector<std::string>> res;
        ozo::async_request(conn_info, "SELECT "_SQL + foos, std::back_inserter(res),
                [&](ozo::error_code ec, auto conn) {
            ASSERT_FALSE(ec) << ec.message() << " | " << error_message(conn) << " | " << get_error_context(conn);
            ASSERT_EQ(1, res.size());
            EXPECT_THAT(std::get<0>(res[0]), ElementsAre("foo", "buzz", "bar"));
            EXPECT_FALSE(ozo::connection_bad(conn));
        });

        io.run();
    }

    SHOULD("return selected int array") {
        using namespace ozo::literals;
        using namespace boost::hana::literals;

        ozo::io_context io;
        ozo::connection_info<> conn_info(io, OZO_PG_TEST_CONNINFO);

        const std::vector<int32_t> foos = {1, 22, 333};

        rows_of<std::vector<int32_t>> res;
        ozo::async_request(conn_info, "SELECT "_SQL + foos, std::back_inserter(res),
                [&](ozo::error_code ec, auto conn) {
            ASSERT_FALSE(ec) << ec.message() << " | " << error_message(conn) << " | " << get_error_context(conn);
            ASSERT_EQ(1, res.size());
            EXPECT_THAT(std::get<0>(res[0]), ElementsAre(1, 22, 333));
            EXPECT_FALSE(ozo::connection_bad(conn));
        });

        io.run();
    }

    SHOULD("fill oid map when oid map is not empty") {
        using namespace ozo::literals;
        namespace asio = boost::asio;

        ozo::io_context io;
        constexpr const auto oid_map = ozo::register_types<ozo::testing::custom_type>();
        ozo::connection_info<> conn_info(io, OZO_PG_TEST_CONNINFO);
        ozo::connection_info<std::decay_t<decltype(oid_map)>> conn_info_with_oid_map(io, OZO_PG_TEST_CONNINFO);

        asio::spawn(io, [&] (asio::yield_context yield) {
            ozo::result result;
            auto conn = ozo::request(conn_info, "DROP TYPE IF EXISTS custom_type"_SQL, result, yield);
            ozo::request(conn, "CREATE TYPE custom_type AS ()"_SQL, result, yield);
            auto conn_with_oid_map = ozo::get_connection(conn_info_with_oid_map, yield);
            EXPECT_NE(ozo::type_oid<ozo::testing::custom_type>(ozo::get_oid_map(conn_with_oid_map)), ozo::null_oid);
        });

        io.run();
    }
}
