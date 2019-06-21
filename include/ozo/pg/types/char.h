#pragma once

#include <ozo/pg/definitions.h>

namespace ozo::pg {
using char_t = char;
}

OZO_PG_BIND_TYPE(ozo::pg::char_t, "char")
