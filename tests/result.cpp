#include "ozo/result.h"
#include "ozo/row.h"
#include "ozo/type_traits.h"
#include "pg_mocks.h"

#include <GUnit/GTest.h>

using namespace ozo;
using namespace ozo::error;

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

GTEST("ozo::result", "[empty set of rows converts to empty_result]") {
    mock_pg_result<0, 0> rows { {} };
    ozo::empty_result empty;
    EXPECT_EQ(ozo::convert_rows(rows, empty), ok);
}

GTEST("ozo::result", "[empty set of rows fails to convert to another type]") {
    mock_pg_result<0, 0> rows { {} };
    int other = 0;
    EXPECT_NE(ozo::convert_rows(rows, other), ok);
}

GTEST("ozo::result", "[multiple rows convert to a type that provides back_inserter via traits]") { EXPECT_TRUE(false); }

