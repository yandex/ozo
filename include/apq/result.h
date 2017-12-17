#pragma once

#include "apq/row.h"
#include "apq/error_code.h"

namespace apq {

struct empty_result {};

template <typename Rows, typename Result>
error_code convert_rows(const Rows&, Result&)
{
    return error_code{};
}

} // namespace apq