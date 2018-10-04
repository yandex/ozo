#pragma once

#include <ozo/type_traits.h>
#include <boost/hana/adapt_struct.hpp>
#include <boost/hana/members.hpp>
#include <boost/hana/size.hpp>
#include <boost/hana/fold.hpp>
#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <boost/range/numeric.hpp>

namespace ozo::detail {

struct pg_array {
    BOOST_HANA_DEFINE_STRUCT(pg_array,
        (std::int32_t, dimensions_count),
        (std::int32_t, dataoffset),
        (::Oid, elemtype)
    );
};

struct pg_array_dimension {
    BOOST_HANA_DEFINE_STRUCT(pg_array_dimension,
        (size_type, size),
        (std::int32_t, index)
    );
};

template <typename T>
struct size_of_array_impl {
    constexpr static size_type data_size(const T& v) {
        using ozo::size_of;
        if constexpr (StaticSize<typename T::value_type>) {
            return std::empty(v) ? 0 : (sizeof(size_type) + size_of(*std::begin(v))) * std::size(v);
        }
        return boost::accumulate(v, size_type(0),
            [&] (auto r, const auto& item) { return r + sizeof(size_type) + size_of(item);});
    }

    static constexpr auto apply(const T& v) {
        constexpr const auto header_size = hana::fold(
            hana::members(pg_array{}), size_type(0),
            [&] (auto r, const auto& item) { return r + sizeof(item); });

        constexpr const auto dimension_header_size = hana::fold(
            hana::members(pg_array_dimension{}), size_type(0),
            [&] (auto r, const auto& item) { return r + sizeof(item); });

        return header_size + dimension_header_size + data_size(v);
    }
};

template <typename T>
struct size_of_impl_dispatcher<T, Require<Array<T>>> { using type = size_of_array_impl<std::decay_t<T>>; };

} // namespace ozo::detail

BOOST_FUSION_ADAPT_STRUCT(ozo::detail::pg_array,
    dimensions_count,
    dataoffset,
    elemtype
)

BOOST_FUSION_ADAPT_STRUCT(ozo::detail::pg_array_dimension,
    size,
    index
)
