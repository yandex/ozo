#pragma once

#include <ozo/pg/definitions.h>
#include <ozo/core/strong_typedef.h>

#include <vector>

namespace ozo::pg {
OZO_STRONG_TYPEDEF(std::vector<char>, bytea)
}

OZO_PG_BIND_TYPE(ozo::pg::bytea, "bytea")
