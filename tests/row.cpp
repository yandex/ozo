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

    value_converter = mock_pg_converter{};
}
