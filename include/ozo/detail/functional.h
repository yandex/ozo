#pragma once

#include <utility>

namespace ozo::detail::functional {

struct forward {
    template <typename T>
    constexpr static decltype(auto) apply(T&& v) noexcept {
        return std::forward<T>(v);
    }
};

struct dereference {
    template <typename T>
    constexpr static decltype(auto) apply(T&& v) noexcept(noexcept(*v)) {
        return *v;
    }
};

struct operator_not {
    template <typename T>
    constexpr static decltype(auto) apply(T&& v) noexcept(noexcept(!v)) {
        return !v;
    }
};

struct always_true {
    template <typename T>
    constexpr static std::true_type apply(T&&) noexcept { return {};}
};

struct always_false {
    template <typename T>
    constexpr static std::false_type apply(T&&) noexcept { return {};}
};

} // namespace ozo::detail::functional
