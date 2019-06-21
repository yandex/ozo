#pragma once

#include <ozo/type_traits.h>
#include <string>

/**
 * @defgroup group-ext-std-string std::string
 * @ingroup group-ext-std
 * @brief [std::string](https://en.cppreference.com/w/cpp/string/basic_string)
 *
 *@code
#include <ozo/ext/std/string.h>
 *@endcode
 *
 * `std::string` is mapped as `text` PostgreSQL type.
 */

OZO_PG_DEFINE_TYPE_AND_ARRAY(std::string, "text", TEXTOID, TEXTARRAYOID, dynamic_size)

/**
 * @defgroup group-ext-std-string_view std::string_view
 * @ingroup group-ext-std
 * @brief [std::string_view](https://en.cppreference.com/w/cpp/string/basic_string_view)
 *
 *@code
#include <ozo/ext/std/string.h>
 *@endcode
 *
 * `std::string_view` is mapped as `text` PostgreSQL type.
 * @note It can be used as a query parameter only!
 */

OZO_PG_DEFINE_TYPE_AND_ARRAY(std::string_view, "text", TEXTOID, TEXTARRAYOID, dynamic_size)
