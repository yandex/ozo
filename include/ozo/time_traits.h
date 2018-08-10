#pragma once

#include <chrono>

namespace ozo {

struct time_traits {
    using duration = std::chrono::steady_clock::duration;
    using time_point = std::chrono::steady_clock::time_point;
};

} // namespace apq
