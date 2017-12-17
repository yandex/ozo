#include "apq/result.h"
#include "apq/row.h"
#include "apq/type_traits.h"

#include <GUnit/GTest.h>

struct mock_pg_value
{
    libapq::oid_t oid_;
    std::string data_;

    libapq::oid_t oid() const { return oid_; }
    const char* bytes() const { return data_.data(); }
    std::size_t size() const { return data_.size(); }
};

template <unsigned NumRows, unsigned NumColumns>
class mock_pg_result
{
public:
    using row_t = std::array<mock_pg_value, NumColumns>;

    mock_pg_result(std::array<row_t, NumRows>&& rows)
    : rows(std::move(rows)) {}

 private:
    std::array<row_t, NumRows> rows;
};

static const apq::error_code success{};

GTEST("libapq::result", "[empty set of rows converts to empty_result]") {
    mock_pg_result<0, 0> rows { {} };
    apq::empty_result empty;
    EXPECT_EQ(apq::convert_rows(rows, empty), success);
}

GTEST("libapq::result", "[empty set of rows fails to convert to another type]") {
    mock_pg_result<0, 0> rows { {} };
    int other = 0;
    EXPECT_NE(apq::convert_rows(rows, other), success);
}

GTEST("libapq::result", "[a single row containing text converts to std::string]") {
    using namespace std::string_literals;
    const auto expected_result = "test"s;

    mock_pg_result<1, 1> rows { {{
        {{ {TEXTOID, expected_result} }}
    }} };
    std::string result;
    EXPECT_EQ(apq::convert_rows(rows, result), success);
    EXPECT_EQ(result, expected_result);
}

GTEST("libapq::result", "[a single row containing int4 converts to int]") {
    using namespace std::string_literals;
    const int expected_result = 777;

    mock_pg_result<1, 1> rows { {{
        {{ {INT4OID, std::to_string(expected_result)} }}
    }} };
    int result;
    EXPECT_EQ(apq::convert_rows(rows, result), success);
    EXPECT_EQ(result, expected_result);    
}

GTEST("libapq::result", "[a single row converts to apq::row]") {
    using namespace std::string_literals;
    mock_pg_result<1, 2> rows { {{
        {{ {INT4OID, std::to_string(42)}, {TEXTOID, "test"s} }}
    }} };  

    apq::row<std::array<mock_pg_value, 2>> result;  
    EXPECT_EQ(apq::convert_rows(rows, result), success);
    EXPECT_EQ(result.at<int>(0), 42);    
    EXPECT_EQ(result.at<std::string>(1), "test"s);    
}

GTEST("libapq::result", "[a single row converts to std::tuple if types match]") { EXPECT_TRUE(false); }
GTEST("libapq::result", "[a single row converts to fusion adapted struct if types match]") { EXPECT_TRUE(false); }
GTEST("libapq::result", "[a single row converts to hana adapted struct if types match]") { EXPECT_TRUE(false); }
GTEST("libapq::result", "[multiple rows convert to a type that provides back_inserter via traits]") { EXPECT_TRUE(false); }

