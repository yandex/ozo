#pragma once

#include <libpq-fe.h>
#include <boost/hana/core/to.hpp>
#include <memory>

namespace ozo::pg {

template <typename T>
struct safe_handle;

template<>
struct safe_handle<::PGresult> {
    struct deleter {
        void operator() (::PGresult *ptr) const noexcept { ::PQclear(ptr); }
    };
    using type = std::unique_ptr<::PGresult, deleter>;
};

template <>
struct safe_handle<::PGconn> {
    struct deleter {
        void operator() (::PGconn *ptr) const noexcept { ::PQfinish(ptr); }
    };
    using type = std::unique_ptr<::PGconn, deleter>;
};

template <>
struct safe_handle<::PGnotify> {
    struct deleter {
        void operator() (::PGnotify *ptr) const noexcept { ::PQfreemem(ptr); }
    };
    using type = std::unique_ptr<::PGnotify, deleter>;
};

template <typename T>
using safe_handle_t = typename safe_handle<T>::type;

template <typename T>
safe_handle_t<T> make_safe(T* handle) noexcept(noexcept(safe_handle_t<T>{handle})) {
    return safe_handle_t<T>{handle};
}

using conn = safe_handle_t<::PGconn>;

using result = pg::safe_handle_t<::PGresult>;

using shared_result = std::shared_ptr<::PGresult>;

using notify = pg::safe_handle_t<::PGnotify>;

using shared_notify = std::shared_ptr<const ::PGnotify>;

} // namespace ozo::pg

namespace boost::hana {

template <>
struct to_impl<ozo::pg::shared_result, ozo::pg::result> {
    static ozo::pg::shared_result apply(ozo::pg::result x) {
        return {x.release(), x.get_deleter()};
    }
};

template <>
struct to_impl<ozo::pg::shared_notify, ozo::pg::notify> {
    static ozo::pg::shared_notify apply(ozo::pg::notify x) {
        return {x.release(), x.get_deleter()};
    }
};

} // namespace boost::hana
