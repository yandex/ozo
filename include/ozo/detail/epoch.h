#pragma once

#include <chrono>

namespace ozo::detail {

// pg epoch is 2000-01-01 00:00:00, which is 30 years (10957 days)
// after the unix epoch (1970-01-01 00:00:00, the default value for a time_point)
// c++20: static constexpr const auto epoch = std::chrono::system_clock::time_point{} + std::chrono::years{ 30 };
static constexpr const auto epoch = std::chrono::system_clock::time_point{} + std::chrono::hours{ 24 } * 10957;

}
