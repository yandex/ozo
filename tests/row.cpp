#include "ozo/row.h"
#include "pg_mocks.h"

#include <GUnit/GTest.h>

using namespace ozo;
using namespace ozo::error;
template <std::size_t Length, typename ValueConverter>
using test_row = basic_row<mock_pg_row<Length>, ValueConverter>;

GTEST("ozo::row") {
    mock_pg_converter value_converter;
    auto converter_func = [&](oid_t oid, const char* bytes, std::size_t size, auto& value) {
        return value_converter(oid, bytes, size, value);
    };

    test_row<1, decltype(converter_func)> r{ {oid_t{}, "123"}, converter_func };
    int unimportant_value;

    SHOULD("[perform value conversion only on demand]") {
        EXPECT_EQ(value_converter.times_called, 0);
        EXPECT_EQ(value_converter.ec, r.at(0, unimportant_value));
        EXPECT_EQ(value_converter.times_called, 1);
    }

    SHOULD("[return index_out_of_range if index is out of range]") {
        EXPECT_EQ(row_index_out_of_range, r.at(1, unimportant_value));
    }

    SHOULD("[return an error if converter returns one]") {
        value_converter.ec = oid_type_mismatch;
        EXPECT_EQ(oid_type_mismatch, r.at(0, unimportant_value));
    }
}

GTEST("ozo::convert_row") {
    mock_pg_converter value_converter;
    auto converter_func = [&](oid_t oid, const char* bytes, std::size_t size, auto& value) {
        return value_converter(oid, bytes, size, value);
    };

    mock_pg_row<2> data{{ {oid_t{}, "123"}, {oid_t{}, "456"} }};

    SHOULD("[convert pg row to a tuple of suitable size]") {
        std::tuple<std::string, int> r;
        EXPECT_EQ(value_converter.ec, convert_row(data, r, converter_func));
        EXPECT_EQ(2, value_converter.times_called);
        EXPECT_EQ("123", std::get<0>(r));
        EXPECT_EQ(456, std::get<1>(r));
    }

    SHOULD("[fail with converter ec if converter returns one]") {
        std::tuple<std::string, int> r;
        value_converter.ec = error::oid_type_mismatch;
        EXPECT_EQ(value_converter.ec, convert_row(data, r, converter_func));
        EXPECT_EQ(1, value_converter.times_called);
    }

    SHOULD("[fail with a row_type_mismatch if target is shorter]") {
        std::tuple<std::string> r;
        EXPECT_EQ(error::row_type_mismatch, convert_row(data, r, converter_func));
    }

    SHOULD("[fail with a row_type_mismatch if target is longer]") {
        std::tuple<std::string, int, int> r;
        EXPECT_EQ(error::row_type_mismatch, convert_row(data, r, converter_func));
    }
}