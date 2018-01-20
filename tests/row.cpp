#include "apq/row.h"
#include "pg_mocks.h"

#include <GUnit/GTest.h>

auto empty_type_map = boost::hana::make_map();

using namespace apq;
template <std::size_t Length>
using test_row = basic_row<mock_pg_row<Length>, mock_pg_converter, decltype(empty_type_map)>;

GTEST("libapq::row") {
    mock_pg_converter::times_called() = 0;
    test_row<1> r{ {oid_t{}, "123"} }; // oid is not important for mock_pg_converter
    int unimportant_value;

    SHOULD("[perform value conversion only on demand]") {
        EXPECT_EQ(mock_pg_converter::times_called(), 0);
        EXPECT_EQ(mock_pg_converter::ec(), r.at(0, unimportant_value));
        EXPECT_EQ(mock_pg_converter::times_called(), 1);
    }

    SHOULD("[return index_out_of_range if index is out of range]") {
        EXPECT_EQ(index_out_of_range, r.at(1, unimportant_value));
    }

    SHOULD("[return an error if converter returns one]") {
        mock_pg_converter::ec() = type_mismatch;
        EXPECT_EQ(type_mismatch, r.at(0, unimportant_value));
    }

    mock_pg_converter::times_called() = 0;
    mock_pg_converter::ec() = success;
}
