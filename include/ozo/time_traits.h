#pragma once

#include <ozo/core/concept.h>
#include <chrono>

namespace ozo {

/**
 * @brief Time traits of the library
 * @ingroup group-core-types
 */
struct time_traits {
    using duration = std::chrono::steady_clock::duration; //!< Time duration type of the library
    using time_point = std::chrono::steady_clock::time_point; //!< Time point type of the library
    /**
     * Get current time
     *
     * @return time_point --- current time
     */
    static time_point now() noexcept(noexcept(std::chrono::steady_clock::now())) {
        return std::chrono::steady_clock::now();
    }
};

template <typename ...Ts>
struct is_time_constraint<std::chrono::duration<Ts...>> : std::true_type {};

template <typename ...Ts>
struct is_time_constraint<std::chrono::time_point<Ts...>> : std::true_type {};

} // namespace ozo
