#pragma once

#include <ozo/type_traits.h>
#include <ozo/core/strong_typedef.h>

#include <string>

namespace ozo::pg {
OZO_STRONG_TYPEDEF(std::string, name)
}

OZO_PG_DEFINE_TYPE_AND_ARRAY(ozo::pg::name, "name", NAMEOID, 1003, dynamic_size)
