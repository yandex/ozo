#pragma once

#include <libpq-fe.h>
#include <memory>

namespace ozo {

struct native_conn_handle_deleter {
    void operator() (::PGconn *ptr) const { ::PQfinish(ptr); }
};

/**
 * @brief libpq PGconn safe RAII representation.
 * libpq PGconn safe RAII representation.
 */
using native_conn_handle = std::unique_ptr<::PGconn, native_conn_handle_deleter>;

} // namespace ozo
