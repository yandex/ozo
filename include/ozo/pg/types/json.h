#pragma once

#include <ozo/type_traits.h>
#include <ozo/core/strong_typedef.h>

#include <string>

namespace ozo::pg {
OZO_STRONG_TYPEDEF(std::string, json)
}

OZO_PG_DEFINE_TYPE_AND_ARRAY(ozo::pg::json, "json", 114, 199, dynamic_size)
