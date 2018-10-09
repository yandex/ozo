#pragma once

#include <ozo/type_traits.h>
#include <ozo/io/send.h>
#include <ozo/io/recv.h>
#include <ozo/io/size_of.h>
#include <boost/hana/adapt_struct.hpp>
#include <boost/hana/members.hpp>
#include <boost/hana/size.hpp>
#include <boost/hana/fold.hpp>
#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <boost/fusion/include/fold.hpp>

namespace ozo::detail {

struct pg_composite {
    BOOST_HANA_DEFINE_STRUCT(pg_composite,
        (std::int32_t, count)
    );
};

} // namespace ozo::detail


BOOST_FUSION_ADAPT_STRUCT(ozo::detail::pg_composite, count)

namespace ozo {

namespace detail {

constexpr auto size_of(const pg_composite&) noexcept {
    return size_constant<sizeof(std::int32_t)>{};
}

template <typename T>
constexpr auto fields_number(const T& v) -> Require<FusionSequence<T>&&!HanaStruct<T>, size_type> {
    return size_type(fusion::size(v));
}

template <typename T>
constexpr auto fields_number(const T& v) -> Require<HanaStruct<T>, size_type> {
    return size_type(hana::value(hana::size(hana::members(v))));
}

template <typename T>
constexpr auto data_size(const T& v) -> Require<FusionSequence<T>&&!HanaStruct<T>, size_type> {
    using ozo::size_of;
    return fusion::fold(v, size_type(0),
        [&] (auto r, const auto& item) { return r + frame_size(item); });
}

template <typename T>
constexpr auto data_size(const T& v)  -> Require<HanaStruct<T>, size_type>{
    using ozo::size_of;
    return hana::fold(hana::members(v), size_type(0),
        [&] (auto r, const auto& item) { return r + frame_size(item); });
}

template <typename T>
struct size_of_composite {
    static constexpr auto apply(const T& v) {
        constexpr const auto header_size = size_of(detail::pg_composite{});
        return header_size +  data_size(v);
    }
};

template <typename T, typename Func>
constexpr auto for_each_member(T&& v, Func&& f) -> Require<FusionSequence<T>&&!HanaStruct<T>> {
    fusion::for_each(std::forward<T>(v), std::forward<Func>(f));
}

template <typename T, typename Func>
constexpr auto for_each_member(T&& v, Func&& f)  -> Require<HanaStruct<T>>{
    hana::for_each(hana::members(std::forward<T>(v)), std::forward<Func>(f));
}

template <typename T>
struct size_of_impl_dispatcher<T, Require<Composite<T>>> { using type = size_of_composite<std::decay_t<T>>; };

template <typename T>
struct send_composite_impl {
    template <typename M>
    static ostream& apply(ostream& out, const oid_map_t<M>& oid_map, const T& in) {
        write(out, pg_composite{fields_number(in)});
        for_each_member(in, [&] (const auto& v) {
            send_frame(out, oid_map, v);
        });
        return out;
    }
};

template <typename T>
struct send_impl_dispatcher<T, Require<Composite<T>>> { using type = send_composite_impl<std::decay_t<T>>; };

template <typename T>
inline Require<Composite<T>> read_and_verify_header(istream& in, const T& v) {
    pg_composite header;
    read(in, header);
    if (header.count != fields_number(v)) {
        throw std::range_error("incoming composite fields count " + std::to_string(header.count)
            + " does not match fields count " + std::to_string(fields_number(v))
            + " of type " + boost::core::demangle(typeid(v).name()));
    }
}

template <typename T>
struct recv_fusion_adapted_composite_impl {
    template <typename M>
    static istream& apply(istream& in, size_type, const oid_map_t<M>& oid_map, T& out) {
        read_and_verify_header(in, out);
        fusion::for_each(out, [&] (auto& v) {
            recv_frame(in, oid_map, v);
        });
        return in;
    }
};

template <typename T>
struct recv_hana_adapted_composite_impl {
    template <typename M>
    static istream& apply(istream& in, size_type, const oid_map_t<M>& oid_map, T& out) {
        read_and_verify_header(in, out);
        out = hana::unpack(
            hana::fold(hana::members(out), hana::tuple<>(),
                [&] (auto&& r, auto&& v) {
                    recv_frame(in, oid_map, v);
                    return hana::append(std::move(r), std::move(v));
                }),
            [] (auto&& ... args) { return T {std::move(args) ...}; }
        );
        return in;
    }
};

template <typename T>
struct recv_impl_dispatcher<T, Require<Composite<T>&&FusionSequence<T>>> {
    using type = recv_fusion_adapted_composite_impl<std::decay_t<T>>;
};

template <typename T>
struct recv_impl_dispatcher<T, Require<Composite<T>&&!FusionSequence<T>>> {
    using type = recv_hana_adapted_composite_impl<std::decay_t<T>>;
};

} // namespace detail
} //namespace ozo
