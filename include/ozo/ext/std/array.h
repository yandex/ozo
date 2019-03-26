#pragma once

#include <ozo/io/array.h>
#include <array>

namespace ozo {

template <typename T, std::size_t size>
struct is_array<std::array<T, size>> : std::true_type {};

template <typename T, std::size_t S>
struct fit_array_size_impl<std::array<T, S>> {
    static void apply(const std::array<T, S>& array, size_type size) {
        if (size != static_cast<size_type>(array.size())) {
            throw system_error(error::bad_array_size,
                "received size " + std::to_string(size)
                + " does not match array size " + std::to_string(array.size()));
        }
    }
};

} // namespace ozo
