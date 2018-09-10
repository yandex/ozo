#pragma once

#include <chrono>

/**
 * @defgroup group-core-time Time
 * @ingroup group-core
 * @brief Database related type system of the library.
 */
namespace ozo {

/**
 * @brief Time traits of the library
 *
 * The structure containts time related types traits.
 * @ingroup group-core-time
 */
struct time_traits {
    using duration = std::chrono::steady_clock::duration; //!< Time duration type of the library
    using time_point = std::chrono::steady_clock::time_point; //!< Time point type of the library
};

} // namespace apq
