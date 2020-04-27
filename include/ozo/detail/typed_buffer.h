#pragma once

#include <ozo/core/concept.h>

namespace ozo::detail {

template <typename T>
union typed_buffer {
    static_assert(std::is_fundamental_v<T>, "T should be fundamental type");

    T typed;
    char raw[sizeof(T)];
};

} // namespace ozo::detail
