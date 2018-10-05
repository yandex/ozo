#pragma once

#include <ozo/type_traits.h>
#include <boost/weak_ptr.hpp>

namespace ozo {

template <typename T>
struct is_nullable<boost::weak_ptr<T>> : std::true_type {};

template <typename T>
struct unwrap_impl<boost::weak_ptr<T>> {
    template <typename T1>
    constexpr static decltype(auto) apply(T1&& v) noexcept(noexcept(*v.lock())) {
        return *v.lock();
    }
};

template <typename T>
struct is_null_impl<boost::weak_ptr<T>> {
    constexpr static auto apply(const boost::weak_ptr<T>& v) noexcept(
            noexcept(is_null(v.lock()))) {
        return is_null(v.lock());
    }
};

} // namespace ozo
