#pragma once

namespace ozo::detail {

struct do_nothing {
    template <typename ... Args>
    void operator ()(Args&& ...) const noexcept {}
};

} // namespace ozo::detail
