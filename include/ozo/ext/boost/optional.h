#pragma once

#include <ozo/type_traits.h>
#include <boost/optional.hpp>

namespace ozo {

template <typename T>
struct is_nullable<boost::optional<T>> : std::true_type {};

template <typename T>
struct unwrap_impl<boost::optional<T>> : detail::functional::dereference {};

} // namespace ozo
