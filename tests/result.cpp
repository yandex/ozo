#include "ozo/result.h"
#include "ozo/row.h"
#include "ozo/type_traits.h"
#include "pg_mocks.h"

#include <GUnit/GTest.h>

#include <libpq-fe.h>

using namespace ozo;
using namespace ozo::error;

GTEST("ozo::convert_rows") {
    std::array<std::string, 2> mock_pg_rows {{
        "unconverted 1",
        "unconverted 2"
    }};

    mock_row_converter<std::string, 2> row_converter { {{
        "converted 1",
        "converted 2"
    }}, {}, {} };

    auto converter_call = [&](auto&& row, auto& result) {
        return row_converter(row, result);
    };

    SHOULD("[call supplied converter and write result to output iterator for each row]") {
        std::array<std::string, 2> result;
        EXPECT_EQ(row_converter.ec, convert_rows(mock_pg_rows, result.begin(), converter_call));
        EXPECT_EQ(row_converter.times_called, 2);
        EXPECT_EQ(row_converter.result, result);
    }

    SHOULD("[fail with converter ec if converter returns one]") {
        std::array<std::string, 2> result;
        row_converter.ec = error::oid_type_mismatch;
        EXPECT_EQ(row_converter.ec, convert_rows(mock_pg_rows, result.begin(), converter_call));
        EXPECT_EQ(row_converter.times_called, 1);
    }

    SHOULD("[accept back insert iterator as output]") {
        std::vector<std::string> result_storage;
        auto row_factory = [](){ return std::string{}; };
        EXPECT_EQ(row_converter.ec, convert_rows(mock_pg_rows,
                std::back_inserter(result_storage), converter_call, row_factory));
        EXPECT_EQ(row_converter.times_called, 2);
        EXPECT_EQ(row_converter.result[0], result_storage[0]);
        EXPECT_EQ(row_converter.result[1], result_storage[1]);
        EXPECT_EQ(result_storage.size(), 2);
    }
}

struct user_row
{
    int a;
    std::string b;
};

BOOST_FUSION_ADAPT_STRUCT(user_row, a, b)

struct pg_value
{
    oid_t oid() const { return oid_; }
    const char* bytes() const { return bytes_; }
    int size() const { return size_; }

    oid_t oid_;
    const char* bytes_;
    int size_;
};

std::vector<std::vector<pg_value>> make_rows(PGresult* pg_result) {
    int n_tuples = PQntuples(pg_result);
    std::vector<std::vector<pg_value>> rows;
    for (int i = 0; i < n_tuples; ++i) {
        rows.emplace_back();
        int n_fields = PQnfields(pg_result);
        for (int j = 0; j < n_fields; ++j) {
            rows.back().emplace_back(pg_value{
                static_cast<oid_t>(PQftype(pg_result, j)),
                PQgetvalue(pg_result, i, j),
                PQgetlength(pg_result, i, j)
            });
        }
    }
    return rows;
}

GTEST("ozo::result") {
    std::string conninfo = "host=localhost port=5432 user=postgres password=123";

    auto connection = PQconnectdb(conninfo.c_str());
    EXPECT_TRUE(connection != nullptr);

    std::string prepare_sql = R"d(
        DROP TABLE IF EXISTS foo;
        CREATE TABLE foo(a int4, b text);
        INSERT INTO foo VALUES (1, 'one'), (2, 'two'), (3, 'three');
    )d";

    auto prepare_result = PQexec(connection, prepare_sql.c_str());
    EXPECT_EQ(PGRES_COMMAND_OK, PQresultStatus(prepare_result))  << PQresultErrorMessage(prepare_result);

    std::string select_sql = R"d(
        SELECT a, b FROM foo WHERE a > 1 ORDER BY a;
    )d";


    auto pg_result = PQexecParams(connection,
                       select_sql.c_str(),
                       0,
                       nullptr,
                       nullptr,
                       nullptr,
                       nullptr,
                       1);
    EXPECT_TRUE(pg_result != nullptr);
    EXPECT_EQ(PGRES_TUPLES_OK, PQresultStatus(pg_result))  << PQresultErrorMessage(pg_result);
    auto pg_rows = make_rows(pg_result);
    EXPECT_EQ(pg_rows.size(), 2);

    SHOULD("[convert returned rows to a vector of adapted structs]") {
        std::vector<user_row> user_result;
        EXPECT_EQ(error_code(ok), convert_rows(pg_rows, std::back_inserter(user_result),
            [](auto&& row_data, auto& row){ return convert_row(std::forward<decltype(row_data)>(row_data), row); },
            [](){ return user_row{}; }));
        EXPECT_EQ(user_result.size(), 2);
        EXPECT_EQ(user_result[0].a, 2);
        EXPECT_EQ(user_result[0].b, "two");
        EXPECT_EQ(user_result[1].a, 3);
        EXPECT_EQ(user_result[1].b, "three");
    }
}