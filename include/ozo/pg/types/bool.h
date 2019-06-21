#pragma once

#include <ozo/type_traits.h>

namespace ozo::pg {
using bool_t = bool;
using boolean = bool_t;
}

OZO_PG_DEFINE_TYPE_AND_ARRAY(ozo::pg::bool_t, "bool", BOOLOID, 1000, bytes<1>)
