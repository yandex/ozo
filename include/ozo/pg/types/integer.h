#pragma once

#include <ozo/pg/definitions.h>

namespace ozo::pg {

using int2 = int16_t;
using smallint = int2;
using int4 = int32_t;
using integer = int4;
using int8 = int64_t;
using bigint = int8;

} // namespace ozo::pg

OZO_PG_BIND_TYPE(ozo::pg::int8, "int8")
OZO_PG_BIND_TYPE(ozo::pg::int4, "int4")
OZO_PG_BIND_TYPE(ozo::pg::int2, "int2")
