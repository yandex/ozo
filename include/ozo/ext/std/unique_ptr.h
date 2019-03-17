#pragma once

#include <ozo/type_traits.h>
#include <memory>

namespace ozo {

template <typename T, typename Deleter>
struct is_nullable<std::unique_ptr<T, Deleter>> : std::true_type {};

template <typename T>
struct allocate_nullable_impl<std::unique_ptr<T>> {
    template <typename Alloc>
    static void apply(std::unique_ptr<T>& out, const Alloc&) {
        out = std::make_unique<T>();
    }
};

template <typename T>
struct unwrap_impl<std::unique_ptr<T>> : detail::functional::dereference {};

} // namespace ozo
