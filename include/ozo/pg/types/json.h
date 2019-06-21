#pragma once

#include <ozo/pg/definitions.h>
#include <ozo/core/strong_typedef.h>

#include <string>

namespace ozo::pg {
OZO_STRONG_TYPEDEF(std::string, json)
}

OZO_PG_BIND_TYPE(ozo::pg::json, "json")
