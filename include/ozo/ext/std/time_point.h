#pragma once

#include <ozo/pg/definitions.h>
#include <ozo/detail/epoch.h>
#include <ozo/io/send.h>
#include <ozo/io/recv.h>

#include <chrono>
#include <iostream>


namespace ozo {


/**
 * @defgroup group-ext-std-chrono-system-clock-time-point std::chrono::system_clock::time_point
 * @ingroup group-ext-std
 * @brief [std::chrono::system_clock::time_point](https://en.cppreference.com/w/cpp/chrono/time_point) support
 *
 *@code
#include <ozo/ext/std/time_point.h>
 *@endcode
 *
 * `std::chrono::system_clock::time_point` is defined as a point in time.
 */

template <>
struct send_impl<std::chrono::system_clock::time_point> {
    template <typename OidMap>
    static ostream& apply(ostream& out, const OidMap&, const std::chrono::system_clock::time_point& in) {
        int64_t value = std::chrono::duration_cast<std::chrono::microseconds>(in - detail::epoch).count();
        return impl::write(out, value);
    }
};

template <>
struct recv_impl<std::chrono::system_clock::time_point> {
    template <typename OidMap>
    static istream& apply(istream& in, size_type, const OidMap&, std::chrono::system_clock::time_point& out) {
        int64_t value;
        impl::read(in, value);
        out = detail::epoch + std::chrono::microseconds{ value };
        return in;
    }
};

}

OZO_PG_BIND_TYPE(std::chrono::system_clock::time_point, "timestamp")
