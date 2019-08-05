#pragma once

#include <libpq-fe.h>
#include <memory>

namespace ozo {

struct native_result_handle_deleter {
    void operator() (::PGresult *ptr) const { ::PQclear(ptr); }
};

/**
 * @brief libpq PGresult safe RAII representation.
 * libpq PGresult safe RAII representation.
 */
using native_result_handle = std::unique_ptr<::PGresult, native_result_handle_deleter>;

} // namespace ozo