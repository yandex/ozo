#pragma once

#include <ozo/type_traits.h>
#include <ozo/core/strong_typedef.h>

#include <vector>

namespace ozo::pg {
OZO_STRONG_TYPEDEF(std::vector<char>, bytea)
}

OZO_PG_DEFINE_TYPE_AND_ARRAY(ozo::pg::bytea, "bytea", BYTEAOID, 1001, dynamic_size)
