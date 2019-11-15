#pragma once

#include <type_traits>

namespace ozo {

/**
 * @brief defines admissibility to use in multithreaded environment without additional synchronization
 *
 * Represents binary state. True enables memory sychronization for underlying types to support
 * safe access in multithreaded environment. False disables memory synchronization.
 * @ingroup group-core-types
 *
 * @tparam enabled --- binary state.
 */
template <bool enabled>
struct thread_safety : std::bool_constant<enabled> {
    constexpr auto operator !() const noexcept {
        return thread_safety<!enabled>{};
    }
};

constexpr thread_safety<true> thread_safe;

} // namespace ozo
