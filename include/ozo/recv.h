#pragma once

#include <ozo/result.h>
#include <ozo/error.h>
#include <ozo/type_traits.h>
#include <ozo/size_of.h>
#include <ozo/concept.h>
#include <ozo/ext/std.h>
#include <ozo/detail/endian.h>
#include <ozo/detail/float.h>
#include <ozo/istream.h>
#include <boost/core/demangle.hpp>
#include <boost/hana/for_each.hpp>
#include <boost/hana/members.hpp>

namespace ozo {
template <int... I>
constexpr std::tuple<boost::mpl::int_<I>...>
make_mpl_index_sequence(std::integer_sequence<int, I...>) {
    return {};
}

template <int N>
constexpr auto make_index_sequence(boost::mpl::int_<N>) {
    return make_mpl_index_sequence(
        std::make_integer_sequence<int, N>{}
    );
}

template <typename Adt, typename Index>
constexpr decltype(auto) member_name(const Adt&, const Index&) {
    return fusion::extension::struct_member_name<Adt, Index::value>::call();
}

template <typename Adt, typename Index>
constexpr decltype(auto) member_value(Adt&& v, const Index&) {
    return fusion::at<Index>(std::forward<Adt>(v));
}

template <typename Out, typename = std::void_t<>>
struct recv_impl {
    template <typename M>
    static istream& apply(istream& in, size_type size, const oid_map_t<M>&, Out& out) {
        if constexpr (DynamicSize<Out>) {
            out.resize(size);
        } else if (size != size_of(out)) {
            throw std::range_error("data size " + std::to_string(size)
                + " does not match type size " + std::to_string(size_of(out)));
        }
        return read(in, out);
    }
};

namespace detail {

template <typename T, typename = std::void_t<>>
struct recv_impl_dispatcher { using type = recv_impl<std::decay_t<T>>; };

template <typename T>
using get_recv_impl = typename recv_impl_dispatcher<unwrap_type<T>>::type;

template <typename M, typename Oid, typename Out>
inline istream& recv(istream& in, Oid oid, size_type size, const oid_map_t<M>& oids, Out& out) {
    static_assert(std::is_same_v<Oid, oid_t>||std::is_same_v<Oid, null_oid_t>,
        "oid must be oid_t or null_oid_t type");

    if constexpr (Nullable<Out>) {
        if (size == null_state_size) {
            reset_nullable(out);
            return in;
        }
    }

    if constexpr (!std::is_same_v<Oid, null_oid_t>) {
        if (!accepts_oid(oids, out, oid)) {
            throw system_error(error::oid_type_mismatch, "unexpected oid "
                + std::to_string(oid) + " for type "
                + boost::core::demangle(typeid(unwrap_type<Out>).name()));
        }
    }

    (void)oid; // Dummy GCC

    if constexpr (Nullable<Out>) {
        init_nullable(out);
    } else if (size == null_state_size) {
        throw std::invalid_argument("unexpected null for type "
            + boost::core::demangle(typeid(out).name()));
    }

    return detail::get_recv_impl<Out>::apply(in, size, oids, unwrap(out));
}

template <typename M, typename Oid, typename Out>
inline istream& recv_data_frame(istream& in, Oid oid, const oid_map_t<M>& oids, Out& out) {
    size_type size = 0;
    read(in, size);
    return recv(in, oid, size, oids, out);
}

} // namespace detail

template <typename M, typename Out>
inline istream& recv(istream& in, oid_t oid, size_type size, const oid_map_t<M>& oids, Out& out) {
    return detail::recv(in, oid, size, oids, out);
}

template <typename M, typename Out>
inline istream& recv_data_frame(istream& in, const oid_map_t<M>& oids, Out& out) {
    return detail::recv_data_frame(in, null_oid, oids, out);
}

template <typename T, typename Tag>
struct recv_impl<strong_typedef_wrapper<T, Tag>> : recv_impl<std::decay_t<T>> {};

template <typename T, typename M, typename Out>
void recv(const value<T>& in, const oid_map_t<M>& oids, Out& out) {
    detail::istreambuf_view sbuf(in.data(), in.size());
    istream s(&sbuf);
    recv(s, in.oid(), (in.is_null() ? null_state_size : in.size()), oids, out);
}

template <typename T, typename M, typename Out>
Require<!FusionSequence<Out> && !FusionAdaptedStruct<Out>>
recv_row(const row<T>& in, const oid_map_t<M>& oid_map, Out& out) {
    if (std::size(in) != 1) {
        throw std::range_error("row size " + std::to_string(std::size(in))
            + " does not equal 1 for single column result");
    }

    recv(*(in.begin()), oid_map, out);
}

template <typename T, typename M, typename Out>
Require<FusionSequence<Out> && !FusionAdaptedStruct<Out>>
recv_row(const row<T>& in, const oid_map_t<M>& oid_map, Out& out) {

    if (static_cast<std::size_t>(fusion::size(out)) != std::size(in)) {
        throw std::range_error("row size " + std::to_string(std::size(in))
            + " does not match sequence " + boost::core::demangle(typeid(out).name())
            + " size " + std::to_string(fusion::size(out)));
    }

    auto i = in.begin();
    fusion::for_each(out, [&](auto& item) {
        recv(*i, oid_map, item);
        ++i;
    });
}

template <typename T, typename M, typename Out>
Require<FusionAdaptedStruct<Out>>
recv_row(const row<T>& in, const oid_map_t<M>& oid_map, Out& out) {

    if (static_cast<std::size_t>(fusion::size(out)) != std::size(in)) {
        throw std::range_error("row size " + std::to_string(std::size(in))
            + " does not match structure " + boost::core::demangle(typeid(out).name())
            + " size " + std::to_string(fusion::size(out)));
    }

    fusion::for_each(make_index_sequence(fusion::size(out)), [&](auto idx) {
        auto i = in.find(member_name(out, idx));
        if (i == in.end()) {
            throw std::range_error(std::string("row does not contain \"")
                + member_name(out, idx) + "\" column for "
                + boost::core::demangle(typeid(out).name()));
        } else {
            recv(*i, oid_map, member_value(out, idx));
        }
    });
}

template <typename T, typename M, typename Out>
Require<ForwardIterator<Out>, Out>
recv_result(const basic_result<T>& in, const oid_map_t<M>& oid_map, Out out) {
    for (auto row : in) {
        recv_row(row, oid_map, *out++);
    }
    return out;
}

template <typename T, typename M, typename Out>
Require<InsertIterator<Out>, Out>
recv_result(const basic_result<T>& in, const oid_map_t<M>& oid_map, Out out) {
    for (auto row : in) {
        typename Out::container_type::value_type v{};
        recv_row(row, oid_map, v);
        *out++ = std::move(v);
    }
    return out;
}

template <typename T, typename M>
basic_result<T>& recv_result(basic_result<T>& in, const oid_map_t<M>&, basic_result<T>& out) {
    out = std::move(in);
    return out;
}

template <typename T, typename M>
basic_result<T>& recv_result(basic_result<T>& in, const oid_map_t<M>& oid_map, std::reference_wrapper<basic_result<T>> out) {
    return recv_result(in, oid_map, out.get());
}

} // namespace ozo
