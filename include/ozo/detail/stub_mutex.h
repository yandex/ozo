#pragma once

namespace ozo::detail {

struct stub_mutex {
    stub_mutex() = default;
    stub_mutex(const stub_mutex&) = delete;
    constexpr void lock() const noexcept {}
    constexpr void unlock() const noexcept {}
};

} // namespace ozo::detail
