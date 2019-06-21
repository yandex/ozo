#pragma once

#include <ozo/type_traits.h>

namespace ozo::pg {

using float8 = double;
using double_precision = float8;
using float4 = float;
using real = float4;

} // namespace ozo::pg

OZO_PG_DEFINE_TYPE_AND_ARRAY(ozo::pg::float8, "float8", FLOAT8OID, 1022, bytes<8>)
OZO_PG_DEFINE_TYPE_AND_ARRAY(ozo::pg::float4, "float4", FLOAT4OID, FLOAT4ARRAYOID, bytes<4>)
