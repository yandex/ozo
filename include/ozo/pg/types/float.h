#pragma once

#include <ozo/pg/definitions.h>

namespace ozo::pg {

using float8 = double;
using double_precision = float8;
using float4 = float;
using real = float4;

} // namespace ozo::pg

OZO_PG_BIND_TYPE(ozo::pg::float8, "float8")
OZO_PG_BIND_TYPE(ozo::pg::float4, "float4")
