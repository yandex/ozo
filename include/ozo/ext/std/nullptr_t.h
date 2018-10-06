#pragma once

#include <ozo/type_traits.h>
#include <ozo/io/send.h>

namespace ozo {

template <>
struct is_nullable<std::nullptr_t> : std::true_type {};

template <>
struct unwrap_impl<std::nullptr_t> : detail::unwrap_with_forward {};

template <>
struct is_null_impl<std::nullptr_t> :
    detail::is_null_always<std::nullptr_t, std::true_type> {};

template <>
struct send_impl<std::nullptr_t> {
    template <typename M, typename T>
    static ostream& apply(ostream& out, M&&, T&&) { return out;}
};

} // namespace ozo

OZO_PG_DEFINE_TYPE(std::nullptr_t, "null", null_oid, decltype(null_state_size))
