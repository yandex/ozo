#pragma once

#include <ozo/io/send.h>
#include <ozo/io/recv.h>
#include <ozo/type_traits.h>

#include <boost/hana/adapt_struct.hpp>

#include <chrono>
#include <cstdint>

/**
 * @defgroup group-ext-std-chrono-duration-microseconds std::chrono::microseconds
 * @ingroup group-ext-std
 * @brief [std::chrono::microseconds](https://en.cppreference.com/w/cpp/chrono/duration) support
 *
 *@code
#include <ozo/ext/std/duration.h>
 *@endcode
 *
 * `std::chrono::microseconds` is mapped as `interval` PostgreSQL type.
 *
 * @note Supported 64-bit microseconds representation values only.
 * @note In case of overflow (underflow) maximum (minimum) valid value is used.
 */

namespace ozo::detail {

struct pg_interval {
    BOOST_HANA_DEFINE_STRUCT(pg_interval,
        (std::int64_t, microseconds),
        (std::int32_t, days),
        (std::int32_t, months)
    );
};

inline pg_interval from_chrono_duration(const std::chrono::microseconds& in) {
    static_assert(
        std::chrono::microseconds::min().count() == std::numeric_limits<std::int64_t>::min() &&
        std::chrono::microseconds::max().count() == std::numeric_limits<std::int64_t>::max(),
        "std::chrono::microseconds tick representation type is not supported"
    );

    using days = std::chrono::duration<std::int32_t, std::ratio<24 * std::chrono::hours::period::num>>;

    return pg_interval{(in % days(1)).count(), std::chrono::duration_cast<days>(in).count(), 0};
}

inline std::chrono::microseconds to_chrono_duration(const pg_interval& interval) {
    static_assert(
        std::chrono::microseconds::min().count() == std::numeric_limits<std::int64_t>::min() &&
        std::chrono::microseconds::max().count() == std::numeric_limits<std::int64_t>::max(),
        "std::chrono::microseconds tick representation type is not supported"
    );

    using std::chrono::microseconds;
    using std::chrono::duration_cast;

    using usecs = std::chrono::duration<std::int64_t, std::micro>;
    using days = std::chrono::duration<std::int64_t, std::ratio<24 * std::chrono::hours::period::num>>;
    using months = std::chrono::duration<std::int32_t, std::ratio<30 * days::period::num>>;

    auto usecs_surplus = usecs(interval.microseconds) % days(1);
    auto days_total = months(interval.months) + days(interval.days) + duration_cast<days>(usecs(interval.microseconds));

    if (days_total < (duration_cast<days>(microseconds::min()) - days(1))
        || ((days_total < days(0)) && ((days_total + days(1)) + usecs_surplus < microseconds::min() + days(1)))) {
        return microseconds::min();
    }

    if ((duration_cast<days>(microseconds::max()) + days(1)) < days_total
        || ((days(0) < days_total) && (microseconds::max() - days(1) < (days_total - days(1)) + usecs_surplus))) {
        return microseconds::max();
    }

    return days_total + usecs_surplus;
}

} // namespace ozo::detail

namespace ozo {

template <>
struct send_impl<std::chrono::microseconds> {
    template <typename OidMap>
    static ostream& apply(ostream& out, const OidMap&, const std::chrono::microseconds& in) {
        static_assert(ozo::OidMap<OidMap>, "OidMap should model ozo::OidMap");

        return write(out, detail::from_chrono_duration(in));
    }
};

template <>
struct recv_impl<std::chrono::microseconds> {
    template <typename OidMap>
    static istream& apply(istream& in, size_type, const OidMap&, std::chrono::microseconds& out) {
        static_assert(ozo::OidMap<OidMap>, "OidMap should model ozo::OidMap");

        detail::pg_interval interval;
        read(in, interval);

        out = detail::to_chrono_duration(interval);

        return in;
    }
};

} // namespace ozo

namespace ozo::definitions {

template <>
struct type<std::chrono::microseconds> : pg::type_definition<decltype("interval"_s)>{};

} // namespace ozo::definitions
