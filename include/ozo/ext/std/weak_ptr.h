#pragma once

#include <ozo/type_traits.h>
#include <memory>

namespace ozo {
/**
 * @defgroup group-ext-std-weak_ptr std::weak_ptr
 * @ingroup group-ext-std
 * @brief [std::weak_ptr](https://en.cppreference.com/w/cpp/memory/weak_ptr) support
 *
 *@code
#include <ozo/ext/std/weak_ptr.h>
 *@endcode
 *
 * `std::weak_ptr<T>` is defined as #Nullable and uses the implementation of `ozo::is_null()` examing the
 * result of `std::weak_ptr::lock()` method call.
 *
 * The `ozo::unwrap()` function is implemented dereferencing the result of `std::weak_ptr::lock()`
 * method call.
 * @note `ozo::unwrap()` from `std::weak_ptr` is not safe for reference holding since no object owning.
 */
///@{
template <typename T>
struct is_nullable<std::weak_ptr<T>> : std::true_type {};

template <typename T>
struct unwrap_impl<std::weak_ptr<T>> {
    template <typename T1>
    constexpr static decltype(auto) apply(T1&& v) noexcept(noexcept(*v.lock())) {
        return *v.lock();
    }
};

template <typename T>
struct is_null_impl<std::weak_ptr<T>> {
    constexpr static auto apply(const std::weak_ptr<T>& v) noexcept(
            noexcept(is_null(v.lock()))) {
        return is_null(v.lock());
    }
};
///@}
} // namespace ozo

