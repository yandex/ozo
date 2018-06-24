#include <ozo/result.h>
#include <ozo/binary_deserialization.h>

#include <GUnit/GTest.h>

namespace {

auto execute_query(const char* query_text, int binary = 1) {
    using scoped_connection = std::unique_ptr<PGconn, void(*)(PGconn*)>;
    auto connection = scoped_connection(PQconnectdb(OZO_PG_TEST_CONNINFO), PQfinish);
    EXPECT_TRUE(connection != nullptr);

    ozo::native_result_handle result(PQexecParams(connection.get(),
                        query_text,
                        0,
                        nullptr,
                        nullptr,
                        nullptr,
                        nullptr,
                        binary));

    EXPECT_EQ(PGRES_TUPLES_OK, PQresultStatus(result.get())) << PQresultErrorMessage(result.get());

    return ozo::result(std::move(result));
}

GTEST("result integration") {

    SHOULD("convert result into tuple from \"select 1::int4, '2';\"") {
        auto result = execute_query("select 1::int4, '2';");
        auto oid_map = ozo::empty_oid_map();
        std::vector<std::tuple<int32_t, std::string>> r;
        ozo::recv_result(result, oid_map, std::back_inserter(r));

        ASSERT_EQ(r.size(), 1);
        EXPECT_EQ(std::get<0>(r[0]), 1);
        EXPECT_EQ(std::get<1>(r[0]), "2");
    }

    SHOULD("convert result into tuple from \"select 42.13::float, 'text';\"") {
        auto result = execute_query("select 42.13::float4, 'text';");
        auto oid_map = ozo::empty_oid_map();
        std::vector<std::tuple<float, std::string>> r;
        ozo::recv_result(result, oid_map, std::back_inserter(r));

        ASSERT_EQ(r.size(), 1);
        EXPECT_EQ(std::get<0>(r[0]), 42.13f);
        EXPECT_EQ(std::get<1>(r[0]), "text");
    }

    SHOULD("convert result with nulls into a tuple with nullables (back inserter)") {
        // boost::scoped_ptr is missing here. It is neither movable nor copyable
        // by design, therefore ozo cannot pass the row instance it constructed
        // during deserialization into back_insert_iterator::operator=.
        // TODO: test that this can be circumvented by adding a custom
        // move-assign operator to the row type, which swaps the scoped_ptr.
        using row = std::tuple<
            boost::optional<int32_t>,
#ifdef __OZO_STD_OPTIONAL
            __OZO_STD_OPTIONAL<float>,
#else
            boost::optional<float>,
#endif
            std::unique_ptr<std::string>,
            boost::shared_ptr<std::vector<char>>,
            std::shared_ptr<std::string>
        >;
        auto result = execute_query("select 7::int4, 42.13::float4, 'text', null, null;");
        auto oid_map = ozo::empty_oid_map();
        std::vector<row> r;
        ozo::recv_result(result, oid_map, std::back_inserter(r));

        ASSERT_EQ(r.size(), 1);
        EXPECT_TRUE(std::get<0>(r[0]));
        EXPECT_EQ(*std::get<0>(r[0]), 7);
        EXPECT_TRUE(std::get<1>(r[0]));
        EXPECT_EQ(*std::get<1>(r[0]), 42.13f);
        EXPECT_TRUE(std::get<2>(r[0]));
        EXPECT_EQ(*std::get<2>(r[0]), "text");
        EXPECT_FALSE(std::get<3>(r[0]));
        EXPECT_FALSE(std::get<4>(r[0]));
    }

    SHOULD("convert result with nulls into a tuple with nullables (preallocated row)") {
        using row = std::tuple<
            boost::optional<int32_t>,
#ifdef __OZO_STD_OPTIONAL
            __OZO_STD_OPTIONAL<float>,
#else
            boost::optional<float>,
#endif
            boost::scoped_ptr<int64_t>,
            std::unique_ptr<std::string>,
            boost::shared_ptr<std::vector<char>>,
            std::shared_ptr<std::string>
        >;
        auto result = execute_query("select 7::int4, 42.13::float4, 77::int8, 'text', null, null;");
        auto oid_map = ozo::empty_oid_map();
        std::vector<row> r(1);
        ozo::recv_result(result, oid_map, std::begin(r));

        ASSERT_EQ(r.size(), 1);
        EXPECT_TRUE(std::get<0>(r[0]));
        EXPECT_EQ(*std::get<0>(r[0]), 7);
        EXPECT_TRUE(std::get<1>(r[0]));
        EXPECT_EQ(*std::get<1>(r[0]), 42.13f);
        EXPECT_TRUE(std::get<2>(r[0]));
        EXPECT_EQ(*std::get<2>(r[0]), 77ll);
        EXPECT_TRUE(std::get<3>(r[0]));
        EXPECT_EQ(*std::get<3>(r[0]), "text");
        EXPECT_FALSE(std::get<4>(r[0]));
        EXPECT_FALSE(std::get<5>(r[0]));
    }
}

} // namespace
