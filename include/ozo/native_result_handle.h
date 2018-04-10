#pragma once

#include <libpq-fe.h>
#include <memory>
/**
 * There are two ways to specify deleter for std::unique_ptr
 * 1. By template parameter
 * 2. By "std::default_delete" template specialization
 *
 * In the first case we lose the default constructor for the pointer.
 * That's why the second way was chosen.
 */
namespace std {

template <>
class default_delete<PGresult> {
public:
    void operator() (PGresult *ptr) const { PQclear(ptr); }
};

} // namespace std

namespace ozo {

using native_result_handle = std::unique_ptr<PGresult>;

} // namespace ozo