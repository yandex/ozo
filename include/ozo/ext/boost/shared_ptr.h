#pragma once

#include <ozo/type_traits.h>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

namespace ozo {
/**
 * @defgroup group-ext-boost-shared_ptr boost::shared_ptr
 * @ingroup group-ext-boost
 * @brief [boost::shared_ptr](https://www.boost.org/doc/libs/1_69_0/libs/smart_ptr/doc/html/smart_ptr.html#shared_ptr) support
 *
 *@code
#include <ozo/ext/boost/shared_ptr.h>
 *@endcode
 *
 * `boost::shared_ptr<T>` is defined as #Nullable and uses the default implementation of `ozo::is_null()`.
 *
 * Function `ozo::allocate_nullable()` implementation is specialized via
 * [boost::allocate_shared()](https://www.boost.org/doc/libs/1_69_0/libs/smart_ptr/doc/html/smart_ptr.html#make_shared).
 *
 * The `ozo::unwrap()` function is implemented via the dereference operator.
 */
///@{
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
///@}
} // namespace ozo
