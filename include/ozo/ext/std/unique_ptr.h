#pragma once

#include <ozo/type_traits.h>
#include <memory>

namespace ozo {
/**
 * @defgroup group-ext-std-unique_ptr std::unique_ptr
 * @ingroup group-ext-std
 * @brief [std::unique_ptr](https://www.std.org/doc/libs/1_69_0/libs/smart_ptr/doc/html/smart_ptr.html#unique_ptr) support
 *
 *@code
#include <ozo/ext/std/unique_ptr.h>
 *@endcode
 *
 * `std::unique_ptr<T>` is defined as #Nullable and uses the default implementation of `ozo::is_null()`.
 *
 * Function `ozo::allocate_nullable()` implementation is specialized via
 * [std::make_unique<T>()](https://en.cppreference.com/w/cpp/memory/unique_ptr/make_unique) so,
 * the allocator argument is ignored.
 *
 * The `ozo::unwrap()` function is implemented via the dereference operator.
 */
///@{
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
///@}
} // namespace ozo
