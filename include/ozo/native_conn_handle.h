#pragma once

#include <libpq-fe.h>
#include <memory>
namespace std {

/**
 * @brief Default deleter for PGconn
 *
 * There are two ways to specify deleter for std::unique_ptr
 * 1. By template parameter
 * 2. By "std::default_delete" template specialization
 *
 * In the first case we lose the default constructor for the pointer.
 * That's why the second way was chosen.
 */
template <>
struct default_delete<PGconn> {
    void operator() (PGconn *ptr) const { PQfinish(ptr); }
};

} // namespace std

namespace ozo {

/**
 * @brief libpq PGconn safe RAII representation.
 * libpq PGconn safe RAII representation.
 */
using native_conn_handle = std::unique_ptr<PGconn>;

} // namespace ozo

