#pragma once

#include <ozo/type_traits.h>

namespace ozo::pg {
using char_t = char;
}

OZO_PG_DEFINE_TYPE_AND_ARRAY(ozo::pg::char_t, "char", CHAROID, 1002, bytes<1>)
