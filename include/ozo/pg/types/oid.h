#pragma once

#include <ozo/type_traits.h>

namespace ozo::pg {
using oid = ozo::oid_t;
}

OZO_PG_DEFINE_TYPE_AND_ARRAY(ozo::pg::oid, "oid", OIDOID, OIDARRAYOID, bytes<4>)
