#pragma once

#include <libpq-fe.h>
#include <memory>

namespace ozo::pg {

template <typename T>
struct safe_handle;

template<>
struct safe_handle<::PGresult> {
    struct deleter {
        void operator() (::PGresult *ptr) const { ::PQclear(ptr); }
    };
    using type = std::unique_ptr<::PGresult, deleter>;
};

template <>
struct safe_handle<::PGconn> {
    struct deleter {
        void operator() (::PGconn *ptr) const { ::PQfinish(ptr); }
    };
    using type = std::unique_ptr<::PGconn, deleter>;
};

template <typename T>
using safe_handle_t = typename safe_handle<T>::type;

template <typename T>
safe_handle_t<T> make_safe(T* handle) { return safe_handle_t<T>{handle};}

using conn = safe_handle_t<::PGconn>;

using result = pg::safe_handle_t<::PGresult>;

} // namespace ozo::pg
