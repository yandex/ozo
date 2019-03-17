#pragma once

#include <ozo/type_traits.h>
#include <boost/scoped_ptr.hpp>

namespace ozo {

template <typename T>
struct is_nullable<boost::scoped_ptr<T>> : std::true_type {};

template <typename T>
struct allocate_nullable_impl<boost::scoped_ptr<T>> {
    template <typename Alloc>
    static void apply(boost::scoped_ptr<T>& out, const Alloc&) {
        out.reset(new T{});
    }
};

template <typename T>
struct unwrap_impl<boost::scoped_ptr<T>> : detail::functional::dereference {};

} // namespace ozo
