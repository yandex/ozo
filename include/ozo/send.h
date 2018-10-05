#pragma once

#include <ozo/concept.h>
#include <ozo/type_traits.h>
#include <ozo/ostream.h>
#include <libpq-fe.h>
#include <type_traits>

namespace ozo {

template <typename In, typename = std::void_t<>>
struct send_impl{
    template <typename M>
    static ostream& apply(ostream& out, const oid_map_t<M>&, const In& in) {
        return write(out, in);
    }
};

namespace detail {

template <typename T, typename = std::void_t<>>
struct send_impl_dispatcher { using type = send_impl<std::decay_t<T>>; };

template <typename T>
using get_send_impl = typename send_impl_dispatcher<unwrap_type<T>>::type;

} // namespace detail

template <class M, class In>
inline ostream& send(ostream& out, const oid_map_t<M>& oid_map, const In& in) {
    return is_null(in) ? out : detail::get_send_impl<In>::apply(out, oid_map, unwrap(in));
}

template <class M, class In>
inline ostream& send_data_frame(ostream& out, const oid_map_t<M>& oid_map, const In& in) {
    write(out, size_of(in));
    return send(out, oid_map, in);
}

template <class M, class In>
inline ostream& send_frame(ostream& out, const oid_map_t<M>& oid_map, const In& in) {
    write(out, type_oid(oid_map, in));
    return send_data_frame(out, oid_map, in);
}

template <typename T, typename Tag>
struct send_impl<detail::strong_typedef_wrapper<T, Tag>> : send_impl<std::decay_t<T>> {};

struct send_null {
    template <typename M, typename T>
    static ostream& apply(ostream& out, M&&, T&&) { return out;}
};

template <>
struct send_impl<std::nullptr_t> : send_null {};
template <>
struct send_impl<__OZO_NULLOPT_T> : send_null {};

} // namespace ozo
