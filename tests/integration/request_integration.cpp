#include <ozo/connection_info.h>
#include <ozo/connection_pool.h>
#include <ozo/query_builder.h>
#include <ozo/request.h>
#include <ozo/execute.h>
#include <ozo/shortcuts.h>
#include <ozo/pg/jsonb.h>

#include <boost/asio/spawn.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace ozo::pg {

static bool operator ==(const pg::jsonb& lhs, const pg::jsonb& rhs) {
    return lhs.raw_string() == rhs.raw_string();
}

static std::ostream& operator <<(std::ostream& s, const pg::jsonb& v) {
    return s << v.raw_string();
}

} // namespace ozo::pg

namespace ozo::tests {

struct custom_type {
    std::int16_t number;
    std::string text;
};

static bool operator == (const custom_type& lhs, const custom_type& rhs) {
    return lhs.number == rhs.number && lhs.text == rhs.text;
}

static std::ostream& operator << (std::ostream& s, const custom_type& v) {
    return s << '(' << v.number << ",\"" << v.text <<"\")";
}

struct with_optional {
    __OZO_STD_OPTIONAL<std::int32_t> value;
};

static bool operator ==(const with_optional& lhs, const with_optional& rhs) {
    return lhs.value == rhs.value;
}

static std::ostream& operator <<(std::ostream& s, const with_optional& v) {
    if (v.value) {
        return s << *v.value;
    } else {
        return s << "none";
    }
}

struct with_jsonb {
    ozo::pg::jsonb value;
};

static bool operator ==(const with_jsonb& lhs, const with_jsonb& rhs) {
    return lhs.value == rhs.value;
}

static std::ostream& operator <<(std::ostream& s, const with_jsonb& v) {
    return s << v.value;
}

} // namespace ozo::tests

BOOST_FUSION_ADAPT_STRUCT(ozo::tests::custom_type, number, text)
BOOST_FUSION_ADAPT_STRUCT(ozo::tests::with_optional, value)
BOOST_FUSION_ADAPT_STRUCT(ozo::tests::with_jsonb, value)

OZO_PG_DEFINE_CUSTOM_TYPE(ozo::tests::custom_type, "custom_type")
OZO_PG_DEFINE_CUSTOM_TYPE(ozo::tests::with_optional, "with_optional")
OZO_PG_DEFINE_CUSTOM_TYPE(ozo::tests::with_jsonb, "with_jsonb")


#define ASSERT_REQUEST_OK(ec, conn)\
    ASSERT_FALSE(ec) << ec.message() \
        << "|" << ozo::error_message(conn) \
        << "|" << ozo::get_error_context(conn) << std::endl

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
        ASSERT_REQUEST_OK(ec, conn);
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
        ASSERT_REQUEST_OK(ec, conn);
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
        ASSERT_REQUEST_OK(ec, conn);
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
        ozo::error_code ec{};
        auto conn = ozo::request(ozo::make_connector(conn_info, io), "DROP TYPE IF EXISTS custom_type"_SQL, std::ref(result), yield[ec]);
        ASSERT_REQUEST_OK(ec, conn);
        ozo::request(conn, "CREATE TYPE custom_type AS ()"_SQL, std::ref(result), yield[ec]);
        ASSERT_REQUEST_OK(ec, conn);
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
        ozo::error_code ec{};
        auto conn = ozo::request(ozo::make_connector(pool, io), "SELECT 1"_SQL, std::ref(result), yield[ec]);
        ASSERT_REQUEST_OK(ec, conn);
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
        ASSERT_REQUEST_OK(ec, conn);
        EXPECT_THAT(res, ElementsAre(std::make_tuple(1)));
        EXPECT_FALSE(ozo::connection_bad(conn));
    });

    io.run();
}

TEST(request, should_return_custom_composite) {
    using namespace ozo::literals;
    namespace asio = boost::asio;

    ozo::io_context io;

    asio::spawn(io, [&] (asio::yield_context yield) {
        [&] {
            const auto conn_info = ozo::make_connection_info(OZO_PG_TEST_CONNINFO);
            ozo::error_code ec{};
            auto conn = ozo::execute(ozo::make_connector(conn_info, io),
                "DROP TYPE IF EXISTS custom_type"_SQL, yield[ec]);
            ASSERT_REQUEST_OK(ec, conn);
            ozo::execute(conn, "CREATE TYPE custom_type AS (number int2, text text)"_SQL, yield[ec]);
            ASSERT_REQUEST_OK(ec, conn);
        }();

        const auto conn_info = ozo::make_connection_info(
            OZO_PG_TEST_CONNINFO,
            ozo::register_types<custom_type>());

        ozo::rows_of<custom_type> out;
        ozo::error_code ec{};
        auto conn = ozo::request(ozo::make_connector(conn_info, io),
            "SELECT * FROM (VALUES ((1, 'one')::custom_type), ((2, 'two')::custom_type)) AS t (tuple);"_SQL,
            ozo::into(out), yield);

        ASSERT_REQUEST_OK(ec, conn);

        EXPECT_THAT(out, ElementsAre(
            std::make_tuple(custom_type{1, "one"}),
            std::make_tuple(custom_type{2, "two"})
        ));

    });

    io.run();
}

TEST(request, should_send_custom_composite) {
    using namespace ozo::literals;
    namespace asio = boost::asio;

    ozo::io_context io;

    asio::spawn(io, [&] (asio::yield_context yield) {
        [&] {
            const auto conn_info = ozo::make_connection_info(OZO_PG_TEST_CONNINFO);
            ozo::error_code ec{};
            auto conn = ozo::execute(ozo::make_connector(conn_info, io),
                "DROP TYPE IF EXISTS custom_type"_SQL, yield[ec]);
            ASSERT_REQUEST_OK(ec, conn);
            ozo::execute(conn, "CREATE TYPE custom_type AS (number int2, text text)"_SQL, yield[ec]);
            ASSERT_REQUEST_OK(ec, conn);
        }();

        const auto conn_info = ozo::make_connection_info(
            OZO_PG_TEST_CONNINFO,
            ozo::register_types<custom_type>());

        ozo::rows_of<custom_type> out;
        ozo::error_code ec{};
        auto conn = ozo::request(ozo::make_connector(conn_info, io),
            "SELECT * FROM (VALUES ("_SQL + custom_type{1, "one"} +
            "), ("_SQL + custom_type{2, "two"} + ")) AS t (tuple);"_SQL,
            ozo::into(out), yield[ec]);

        ASSERT_REQUEST_OK(ec, conn);

        EXPECT_THAT(out, ElementsAre(
            std::make_tuple(custom_type{1, "one"}),
            std::make_tuple(custom_type{2, "two"})
        ));

    });

    io.run();
}

TEST(result, should_send_bytea_properly) {
    using namespace ozo::literals;
    using namespace boost::hana::literals;

    ozo::io_context io;
    ozo::connection_info<> conn_info(OZO_PG_TEST_CONNINFO);

    std::vector<std::tuple<ozo::pg::bytea>> res;
    const std::string foo = "foo";
    auto arr = ozo::pg::bytea({0,1,2,3,4,5,6,7,8,9,0,0});
    EXPECT_EQ(std::size(arr.get())* sizeof(decltype(*arr.get().begin())), ozo::size_of(arr));
    ozo::request(ozo::make_connector(conn_info, io), "SELECT "_SQL + arr, std::back_inserter(res),
            [&](ozo::error_code ec, auto conn) {
        ASSERT_FALSE(ec);
        ASSERT_TRUE(conn);
        ASSERT_EQ(1u, res.size());
        ASSERT_EQ(std::get<0>(res[0]).get(), std::vector<char>({0,1,2,3,4,5,6,7,8,9,0,0}));
    });

    io.run();
}

TEST(request, should_send_empty_optional) {
    using namespace ozo::literals;
    using namespace std::string_literals;
    using namespace std::string_view_literals;
    namespace asio = boost::asio;

    ozo::io_context io;
    const ozo::connection_info<> conn_info(OZO_PG_TEST_CONNINFO);
    const auto timeout = ozo::time_traits::duration::max();
    __OZO_STD_OPTIONAL<std::int32_t> value;

    ozo::rows_of<__OZO_STD_OPTIONAL<std::int32_t>> result;
    std::atomic_flag called {};
    ozo::request(ozo::make_connector(conn_info, io), "SELECT "_SQL + value + "::integer"_SQL, timeout, ozo::into(result),
            [&](ozo::error_code ec, auto conn) {
        EXPECT_FALSE(called.test_and_set());
        ASSERT_REQUEST_OK(ec, conn);
        EXPECT_THAT(result, ElementsAre(value));
        EXPECT_FALSE(ozo::connection_bad(conn));
    });

    io.run();
}

TEST(request, should_send_and_receive_empty_optional) {
    using namespace ozo::literals;
    using namespace std::string_literals;
    using namespace std::string_view_literals;
    namespace asio = boost::asio;

    ozo::io_context io;
    const ozo::connection_info<> conn_info(OZO_PG_TEST_CONNINFO);
    const auto timeout = ozo::time_traits::duration::max();
    __OZO_STD_OPTIONAL<std::int32_t> value;

    ozo::rows_of<__OZO_STD_OPTIONAL<std::int32_t>> result;
    std::atomic_flag called {};
    ozo::request(ozo::make_connector(conn_info, io), "SELECT "_SQL + value + "::integer"_SQL, timeout, ozo::into(result),
            [&](ozo::error_code ec, auto conn) {
        EXPECT_FALSE(called.test_and_set());
        ASSERT_REQUEST_OK(ec, conn);
        EXPECT_THAT(result, ElementsAre(value));
        EXPECT_FALSE(ozo::connection_bad(conn));
    });

    io.run();
}

TEST(request, should_send_and_receive_composite_with_empty_optional) {
    using namespace ozo::literals;
    namespace asio = boost::asio;

    ozo::io_context io;

    asio::spawn(io, [&] (asio::yield_context yield) {
        [&] {
            const auto conn_info = ozo::make_connection_info(OZO_PG_TEST_CONNINFO);
            ozo::error_code ec;
            auto conn = ozo::execute(ozo::make_connector(conn_info, io),
                "DROP TYPE IF EXISTS with_optional"_SQL, yield[ec]);
            ASSERT_REQUEST_OK(ec, conn);
            ozo::execute(conn, "CREATE TYPE with_optional AS (value integer)"_SQL, yield[ec]);
            ASSERT_REQUEST_OK(ec, conn);
        } ();

        const auto conn_info = ozo::make_connection_info(
            OZO_PG_TEST_CONNINFO,
            ozo::register_types<with_optional>()
        );

        const with_optional value;
        ozo::rows_of<with_optional> result;
        ozo::error_code ec;
        auto conn = ozo::request(ozo::make_connector(conn_info, io), "SELECT "_SQL + value + "::with_optional"_SQL,
                                 ozo::into(result), yield[ec]);

        ASSERT_REQUEST_OK(ec, conn);

        EXPECT_THAT(result, ElementsAre(value));
    });

    io.run();
}

TEST(request, should_send_and_receive_jsonb) {
    using namespace ozo::literals;
    using namespace std::string_literals;
    using namespace std::string_view_literals;
    namespace asio = boost::asio;

    ozo::io_context io;
    const ozo::connection_info<> conn_info(OZO_PG_TEST_CONNINFO);
    const auto timeout = ozo::time_traits::duration::max();
    std::string value = R"({"foo": "bar"})";

    ozo::rows_of<ozo::pg::jsonb> result;
    std::atomic_flag called {};
    ozo::request(ozo::make_connector(conn_info, io), "SELECT "_SQL + value + "::jsonb"_SQL, timeout, ozo::into(result),
            [&](ozo::error_code ec, auto conn) {
        EXPECT_FALSE(called.test_and_set());
        ASSERT_REQUEST_OK(ec, conn);
        EXPECT_THAT(result, ElementsAre(ozo::pg::jsonb(value)));
        EXPECT_FALSE(ozo::connection_bad(conn));
    });

    io.run();
}

TEST(request, should_send_and_receive_composite_with_jsonb_field) {
    using namespace ozo::literals;
    using namespace std::string_literals;
    using namespace std::string_view_literals;
    namespace asio = boost::asio;

    ozo::io_context io;

    asio::spawn(io, [&] (asio::yield_context yield) {
        [&] {
            const auto conn_info = ozo::make_connection_info(OZO_PG_TEST_CONNINFO);
            ozo::error_code ec;
            auto conn = ozo::execute(ozo::make_connector(conn_info, io),
                "DROP TYPE IF EXISTS with_jsonb"_SQL, yield[ec]);
            ASSERT_REQUEST_OK(ec, conn);
            ozo::execute(conn, "CREATE TYPE with_jsonb AS (value jsonb)"_SQL, yield[ec]);
            ASSERT_REQUEST_OK(ec, conn);
        } ();

        const auto conn_info = ozo::make_connection_info(
            OZO_PG_TEST_CONNINFO,
            ozo::register_types<with_jsonb>()
        );

        const with_jsonb value {{R"({"foo": "bar"})"}};
        ozo::rows_of<with_jsonb> result;
        ozo::error_code ec;
        auto conn = ozo::request(ozo::make_connector(conn_info, io), "SELECT "_SQL + value + "::with_jsonb"_SQL,
                                 ozo::into(result), yield[ec]);

        ASSERT_REQUEST_OK(ec, conn);

        EXPECT_THAT(result, ElementsAre(with_jsonb {{R"({"foo": "bar"})"}}));
    });

    io.run();
}

} // namespace
