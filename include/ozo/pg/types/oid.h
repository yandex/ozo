#pragma once

#include <ozo/pg/definitions.h>

namespace ozo::pg {
using oid = ozo::oid_t;
}

OZO_PG_BIND_TYPE(ozo::pg::oid, "oid")
