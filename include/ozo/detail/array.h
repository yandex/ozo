#pragma once

#include <ozo/type_traits.h>
#include <boost/hana/adapt_struct.hpp>
#include <boost/fusion/adapted/struct/adapt_struct.hpp>

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
