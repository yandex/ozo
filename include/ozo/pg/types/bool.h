#pragma once

#include <ozo/pg/definitions.h>

namespace ozo::pg {
using bool_t = bool;
using boolean = bool_t;
}

OZO_PG_BIND_TYPE(ozo::pg::bool_t, "bool")
