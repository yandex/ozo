#pragma once

#include <ozo/core/none.h>
#include <ozo/time_traits.h>

namespace ozo {
/**
 * @brief Dealdine calculation
 *
 * Calculate deadline from time point. Literally returns it's argument.
 *
 * @param t --- time point of a deadline
 * @return it's argument
 * @ingroup group-core-functions
 */
inline constexpr time_traits::time_point deadline(time_traits::time_point t) noexcept {
    return t;
}

/**
 * @brief Dealdine calculation
 *
 * Calculates deadline time point in a given duration from a given time point.
 * The result value range is [now, time_traits::time_point::max()].
 * Literally: `now  + after`.
 *
 * @param after --- duration to a deadline time point
 * @param now --- start time point for the duration
 * @return calculated deadline time point
 * @ingroup group-core-functions
 */
inline constexpr time_traits::time_point deadline(
        time_traits::duration after, time_traits::time_point now) noexcept {
    if (after < time_traits::duration(0)) {
        return now;
    }
    return after > time_traits::time_point::max() - now ?
            time_traits::time_point::max() : now + after;
}

/**
 * @brief Dealdine calculation
 *
 * Calculates deadline time point in a given duration from now.
 * The result value is limited by time_traits::time_point::max().
 * Literally: `time_traits::now() + after`.
 *
 * @param after --- duration to a deadline time point
 * @return calculated deadline time point
 * @ingroup group-core-functions
 */
inline time_traits::time_point deadline(time_traits::duration after) noexcept {
    return deadline(after, time_traits::now()) ;
}

/**
 * @brief Dealdine calculation
 *
 * Calculates deadline from `ozo::none_t`.
 *
 * @param none_t
 * @return `ozo::none`
 * @ingroup group-core-functions
 */
inline constexpr auto deadline(none_t) noexcept { return none;}

/**
 * @brief Time left to deadline
 *
 * Calculates time left form given `now` time point to a given deadline time point `t`.
 *
 * @param t --- deadline time point
 * @param now --- time point to calculate duration from
 * @return time left to the time point which is more or equal to a zero.
 * @ingroup group-core-functions
 */
inline constexpr time_traits::duration time_left(
        time_traits::time_point t, time_traits::time_point now) noexcept {
    return t > now ? t - now : time_traits::duration(0);
}

/**
 * @brief Time left to deadline
 *
 * Calculates time left to a given deadline time point.
 *
 * @param t --- deadline time point
 * @return time left to the time point which is more or equal to a zero.
 * @ingroup group-core-functions
 */
inline time_traits::duration time_left(time_traits::time_point t) noexcept {
    return time_left(t, time_traits::now());
}

/**
 * @brief Deadline is expired
 *
 * Indicates if a given dedline is expired for a given time point. Deadline is expired then time
 * left duration to the deadline is 0.
 *
 * @param t --- deadline time point
 * @return true --- deadline has been expired
 * @return false --- there is time left to deadline
 * @ingroup group-core-functions
 */
inline bool expired(time_traits::time_point t, time_traits::time_point now) noexcept {
    return time_left(t, now) == time_traits::duration(0);
}

/**
 * @brief Deadline is expired
 *
 * Indicates if a given dedline is expired. Deadline is expired then time
 * left duration to the deadline is 0.
 *
 * @param t --- deadline time point
 * @return true --- deadline has been expired
 * @return false --- there is time left to deadline
 * @ingroup group-core-functions
 */
inline bool expired(time_traits::time_point t) noexcept {
    return expired(t, time_traits::now());
}

} // namespace ozo
