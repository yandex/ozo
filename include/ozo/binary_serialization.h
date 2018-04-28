#pragma once

#include <ozo/concept.h>
#include <ozo/type_traits.h>
#include <ozo/ostream.h>
#include <ozo/detail/array.h>

#include <libpq-fe.h>
#include <boost/range/algorithm/for_each.hpp>
#include <type_traits>

namespace ozo {

template <typename In, typename = std::void_t<>>
struct send_impl{
    template <typename M>
    static ostream& apply(ostream& out, const oid_map_t<M>&, const In& in) {
        return write(out, in);
    }
};

template <class M, class In>
inline ostream& send(ostream& out, const oid_map_t<M>& oid_map, const In& in) {
    return send_impl<In>::apply(out, oid_map, in);
}

template <typename T, typename Alloc>
struct send_impl<std::vector<T, Alloc>> {
    template <typename M>
    static ostream& apply(ostream& out, const oid_map_t<M>& oid_map, const std::vector<T, Alloc>& in) {
        using value_type = std::decay_t<T>;
        write(out, detail::pg_array {1, 0, type_oid<value_type>(oid_map)});
        write(out, detail::pg_array_dimension {std::int32_t(std::size(in)), 0});
        boost::for_each(in,
            [&] (const auto& v) {
                write(out, std::int32_t(size_of(v)));
                send(out, oid_map, v);
            });
        return out;
    }
};

template <typename Alloc>
struct send_impl<std::vector<char, Alloc>> {
    template <typename M>
    static ostream& apply(ostream& out, const oid_map_t<M>&, const std::vector<char, Alloc>& in) {
        return write(out, in);
    }
};

template <>
struct send_impl<boost::uuids::uuid> {
    template <typename M>
    static ostream& apply(ostream& out, const oid_map_t<M>&, const boost::uuids::uuid& in) {
        return write(out, in);
    }
};

} // namespace ozo
