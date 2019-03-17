#pragma once

#include <ozo/type_traits.h>
#include <memory>

namespace ozo {

template <typename T>
struct is_nullable<std::shared_ptr<T>> : std::true_type {};

template <typename T>
struct allocate_nullable_impl<std::shared_ptr<T>> {
    template <typename Alloc>
    static void apply(std::shared_ptr<T>& out, const Alloc& a) {
        out = std::allocate_shared<T, Alloc>(a);
    }
};

template <typename T>
struct unwrap_impl<std::shared_ptr<T>> : detail::functional::dereference {};

} // namespace ozo
