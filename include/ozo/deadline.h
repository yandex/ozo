#pragma once

#include <ozo/time_traits.h>

namespace ozo {

/**
 * @brief Operation deadline
 * @ingroup group-core-types
 *
 * Most of the operations in real world should be limited in time.
 * There are two cases to specify this limit: time-out and deadline.
 * Timeout is simple but hard to use in case of a complex operation -
 * it needs to calculate timeout for every part of the operation.
 * On the other side - deadline a little bit more complicated but very
 * convenient for the complex operation. Every part just need to meet
 * a deadline condition. Very simple. Lucky, with std::chrono we can
 * easily implement the deadline and specify it through timeout.
 */
class deadline {
public:
    using duration = time_traits::duration; //!< time duration type
    using time_point = time_traits::time_point; //!< time point type

    /**
     * Get current time point, calls ozo::time_traits::now().
     *
     * @return time_point --- current time point.
     */
    static time_point now() noexcept(noexcept(time_traits::now())) {
        return time_traits::now();
    }

    /**
     * Get time left to deadline from specified time point.
     *
     * @param now --- time point to calculate from
     * @return duration --- time left to deadline
     */
    constexpr duration time_left(time_point now) const noexcept {
        return std::max(duration(expiry() - now), duration(0));
    }
    /**
     * Get time left to deadline from now.
     *
     * @return duration --- time left to deadline
     */
    duration time_left() const noexcept(noexcept(now())) {
        return time_left(now());
    }
    /**
     * Get the deadline expiry time as an absolute time.
     *
     * @return time_point --- time point of the deadline
     */
    constexpr time_point expiry() const noexcept { return v_;}
    /**
     * Converts deadline to time_point type via expiry()
     */
    constexpr operator time_point () const noexcept { return expiry();}
    /**
     * Indicates if deadline is expired for the specified time point
     *
     * @param now --- time point to examine
     * @return true --- specified time point is after the deadline
     * @return false --- specified time point is before deadline
     */
    constexpr bool expired(time_point now) const noexcept {
        return expiry() < now;
    }
    /**
     * Indicates if the deadline is expired for now
     *
     * @return true --- deadline has been expired
     * @return false --- deadline has not been reached
     */
    bool expired() const noexcept(noexcept(now())) { return expired(now());}
    /**
     * Indicates if the deadline is expired for now, same as expired()
     */
    bool operator ! () const noexcept(noexcept(expired())) { return expired();}
    /**
     * Indicates if the deadline has not been reached
     */
    operator bool () const noexcept(noexcept(operator !())) { return !!(*this);}

    /**
     * Create a deadline which should be expired at the specified time point.
     * This is time point based deadline constructor.
     *
     * @param at --- deadline time point
     */
    constexpr explicit deadline(time_point at) : v_(at) {}
    /**
     * Create a deadline which should be expired after the specified duration starting from a time point.
     *
     * @param after --- duration after the time point
     * @param now --- time point to calculate deadline from
     */
    constexpr explicit deadline(duration after, time_point now)
    : deadline(now + after) {}
    /**
     * Create a deadline which should be expired after the specified duration from now.
     * This is time-out based deadline constructor.
     *
     * @param after --- duration to the deadline time point from now
     */
    explicit deadline(duration after) : deadline(after, now()) {}
private:
    time_point v_;
};

} // namespace ozo
