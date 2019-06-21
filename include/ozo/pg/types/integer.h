#pragma once

#include <ozo/type_traits.h>

namespace ozo::pg {

using int2 = int16_t;
using smallint = int2;
using int4 = int32_t;
using integer = int4;
using int8 = int64_t;
using bigint = int8;

} // namespace ozo::pg

OZO_PG_DEFINE_TYPE_AND_ARRAY(ozo::pg::int8, "int8", INT8OID, 1016, bytes<8>)
OZO_PG_DEFINE_TYPE_AND_ARRAY(ozo::pg::int4, "int4", INT4OID, INT4ARRAYOID, bytes<4>)
OZO_PG_DEFINE_TYPE_AND_ARRAY(ozo::pg::int2, "int2", INT2OID, INT2ARRAYOID, bytes<2>)
