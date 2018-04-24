#pragma once

#include <boost/system/system_error.hpp>

namespace ozo {

using error_code = ::boost::system::error_code;
using system_error = ::boost::system::system_error;
using error_category = ::boost::system::error_category;

namespace error {

enum code {
    ok, // to do not use error code 0
    pq_connection_start_failed, // PQConnectionStart function failed
    pq_socket_failed, // PQSocket returned -1 as fd - it seems like there is no connection
    pq_connection_status_bad, // PQstatus returned CONNECTION_BAD
    pq_connect_poll_failed, // PQconnectPoll function failed
    oid_type_mismatch, // no conversion possible from oid to user-supplied type
    unexpected_eof, // unecpected EOF while data read
};

const error_category& category() noexcept;

} // namespace error
} // namespace ozo

namespace boost {
namespace system {

template <>
struct is_error_code_enum<ozo::error::code> : std::true_type {};

} // namespace system
} // namespace boost

namespace ozo {
namespace error {

inline auto make_error_code(const code e) {
    return boost::system::error_code(static_cast<int>(e), category());
}

namespace impl {

class category : public error_category {
public:
    const char* name() const throw() { return "ozo::error::category"; }

    std::string message(int value) const {
        switch (code(value)) {
            case ok:
                return "no error";
            case pq_connection_start_failed:
                return "pq_connection_start_failed - PQConnectionStart function failed";
            case pq_socket_failed:
                return "pq_socket_failed - PQSocket returned -1 as fd - it seems like there is no connection";
            case pq_connection_status_bad:
                return "pq_connection_status_bad - PQstatus returned CONNECTION_BAD";
            case pq_connect_poll_failed:
                return "pq_connect_poll_failed - PQconnectPoll function failed";
            case oid_type_mismatch:
                return "no conversion possible from oid to user-supplied type";
            case unexpected_eof:
                return "unecpected EOF while data read";
        }
        return "no message for value: " + std::to_string(value);
    }
};

} // namespace impl

inline const error_category& category() noexcept {
    static impl::category instance;
    return instance;
}

} // namespace error
} // namespace ozo
