#pragma once

#include <ozo/type_traits.h>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

namespace ozo {

template <typename T>
struct is_nullable<boost::shared_ptr<T>> : std::true_type {};

template <typename T>
struct allocate_nullable_impl<boost::shared_ptr<T>> {
    template <typename Alloc>
    static void apply(boost::shared_ptr<T>& out, const Alloc& a) {
        out = boost::allocate_shared<T, Alloc>(a);
    }
};

template <typename T>
struct unwrap_impl<boost::shared_ptr<T>> : detail::functional::dereference {};

} // namespace ozo
