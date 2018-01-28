#pragma once

#include "ozo/row.h"
#include "ozo/error.h"

namespace ozo {

struct empty_result {};

template <typename Rows, typename Result>
error_code convert_rows(const Rows&, Result&)
{
    return error::ok;
}

} // namespace ozo