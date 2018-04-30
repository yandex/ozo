#pragma once

#include <ozo/result.h>
#include <ozo/error.h>
#include <ozo/type_traits.h>
#include <ozo/concept.h>
#include <ozo/detail/endian.h>
#include <ozo/detail/float.h>
#include <ozo/detail/array.h>
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

template <typename Out>
bool recv_null(bool is_null, Out& out) {
    if constexpr (Nullable<Out>) {
        if (is_null) {
            reset_nullable(out);
            return true;
        }
        init_nullable(out);
    } else {
        if (is_null) {
            throw std::invalid_argument("unexpected null for type "
                + boost::core::demangle(typeid(out).name()));
        }
    }
    return false;
}

template <typename Out, typename = std::void_t<>>
struct recv_impl{
    template <typename M>
    static istream& apply(istream& in, int32_t, const oid_map_t<M>&, Out& out) {
        return read(in, out);
    }
};

template <typename M, typename Out>
inline istream& recv(istream& in, int32_t size, const oid_map_t<M>& oids, Out& out) {
    if constexpr (!is_dynamic_size<Out>::value) {
        if (size != static_cast<int32_t>(size_of(out))) {
            throw std::range_error("data size " + std::to_string(size)
                + " does not match type size " + std::to_string(size_of(out)));
        }
    }
    return recv_impl<std::decay_t<Out>>::apply(in, size, oids, out);
}

template <>
struct recv_impl<std::string> {
    template <typename M>
    static istream& apply(istream& in, int32_t size, const oid_map_t<M>&, std::string& out) {
        out.resize(size);
        return read(in, out);
    }
};

template <typename Alloc>
struct recv_impl<std::vector<char, Alloc>> {
    template <typename M>
    static istream& apply(istream& in, int32_t size, const oid_map_t<M>&, std::vector<char, Alloc>& out) {
        out.resize(size);
        return read(in, out);
    }
};

template <typename Out, typename Alloc>
struct recv_impl<std::vector<Out, Alloc>> {
    using value_type = std::vector<Out, Alloc>;

    template <typename M>
    static istream& apply(istream& in, int32_t, const oid_map_t<M>& oids, value_type& out) {
        detail::pg_array array_header;
        detail::pg_array_dimension dim_header;

        read(in, array_header);

        if (array_header.dimensions_count > 1) {
            throw std::range_error("multiply dimension count is not supported: "
                 + std::to_string(array_header.dimensions_count));
        }

        using item_type = typename value_type::value_type;
        using unwrapped_item_type = unwrap_nullable_type<item_type>;

        if (!accepts_oid<unwrapped_item_type>(oids, array_header.elemtype)) {
            throw system_error(error::oid_type_mismatch,
                "unexpected oid " + std::to_string(array_header.elemtype)
                + " for element type of " + boost::core::demangle(typeid(unwrapped_item_type).name()));
        }

        if (array_header.dimensions_count < 1) {
            return in;
        }

        read(in, dim_header);

        if (dim_header.size == 0) {
            return in;
        }

        out.resize(dim_header.size);

        for (auto& item : out) {
            int32_t size = 0;
            read(in, size);
            const bool is_null = size == -1;
            if (!recv_null(is_null, item)) {
                recv(in, size, oids, unwrap_nullable(item));
            }
        }
        return in;
    }
};

template <>
struct recv_impl<name_oid> {
    template <typename M>
    static istream& apply(istream& in, int32_t size, const oid_map_t<M>& oid_map, name_oid& out) {
        return recv_impl<decltype(out.value)>::apply(in, size, oid_map, out.value);
    }
};

template <typename T, typename M, typename Out>
void recv(const value<T>& in, const oid_map_t<M>& oids, Out& out) {
    if (recv_null(in.is_null(), out)) {
        return;
    }

    if (!accepts_oid(oids, unwrap_nullable(out), in.oid())) {
        throw system_error(error::oid_type_mismatch, "unexpected oid "
            + std::to_string(in.oid()) + " for type "
            + boost::core::demangle(typeid(unwrap_nullable_type<Out>).name()));
    }

    detail::istreambuf_view sbuf(in.data(), in.size());
    istream s(&sbuf);
    recv(s, in.size(), oids, unwrap_nullable(out));
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

} // namespace ozo
