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
}

} // namespace
