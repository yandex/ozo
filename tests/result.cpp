#include "ozo/result.h"
#include "ozo/row.h"
#include "ozo/type_traits.h"
#include "pg_mocks.h"

#include <GUnit/GTest.h>

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
