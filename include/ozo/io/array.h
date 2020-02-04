#pragma once

#include <ozo/type_traits.h>
#include <ozo/io/send.h>
#include <ozo/io/recv.h>
#include <boost/hana/adapt_struct.hpp>
#include <boost/hana/size.hpp>
#include <boost/hana/fold.hpp>
#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <boost/range/numeric.hpp>
#include <boost/range/algorithm/for_each.hpp>

namespace ozo {

template <typename T>
struct fit_array_size_impl {
    static void apply(T& array, size_type count) { array.resize(count); }
};

/**
 * @brief Fits array container size to reqired one
 * @ingroup group-io-functions
 *
 * Function is used to request container which represents an array to
 * to be able to store requested count of elements. The function may
 * throw exception if the container can not accept the requested
 * count of elements (see the example below).
 *
 * @param array --- container which represents an array
 * @param count --- required count of elements to be able to contain
 *
 * The implementation can be customized with `ozo::fit_array_size_impl`
 * template specialization. By default it uses `resize()` function of
 * the array container and default implementation may look like this
 *
 * @code
template <typename T>
struct fit_array_size_impl {
    static void apply(T& array, size_type count) {
        array.resize(count);
    }
};
 * @endcode
 *
 * E.g. the implementation for a fixed size container like `std::array` may
 * be equal to:
 *
 * @code
template <typename T, std::size_t S>
struct fit_array_size_impl<std::array<T, S>> {
    static void apply(const std::array<T, S>& array, size_type size) {
        if (size != array.size()) {
            throw ozo::system_error(ozo::error::bad_array_size);
        }
    }
};
 * @endcode
 */
template <typename T>
inline void fit_array_size(T& array, size_type count) {
    fit_array_size_impl<T>::apply(array, count);
}

} // namespace ozo

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

} //namespace ozo::detail

namespace ozo::detail {

template <typename T>
struct size_of_array_impl {
    constexpr static size_type data_size(const T& v) {
        using ozo::size_of;
        if constexpr (StaticSize<typename T::value_type>) {
            return std::empty(v) ? 0 : data_frame_size(*std::begin(v)) * std::size(v);
        }
        return boost::accumulate(v, size_type(0),
            [&] (auto r, const auto& item) { return r + data_frame_size(item);});
    }

    static constexpr auto apply(const T& v) {
        constexpr const auto header_size = hana::unpack(
            hana::members(pg_array{}),
            [] (const auto& ...x) { return (sizeof(x) + ... + 0); });

        constexpr const auto dimension_header_size = hana::unpack(
            hana::members(pg_array_dimension{}),
            [] (const auto& ...x) { return (sizeof(x) + ... + 0); });

        return header_size + dimension_header_size + data_size(v);
    }
};

template <typename T>
struct size_of_impl_dispatcher<T, Require<Array<T>>> { using type = size_of_array_impl<std::decay_t<T>>; };

template <typename T>
struct send_array_impl {
    template <typename OidMap>
    static ostream& apply(ostream& out, const OidMap& oid_map, const T& in) {
        using value_type = typename T::value_type;
        write(out, pg_array {1, 0, type_oid<value_type>(oid_map)});
        write(out, pg_array_dimension {std::int32_t(std::size(in)), 0});
        boost::for_each(in, [&] (const auto& v) { send_data_frame(out, oid_map, v);});
        return out;
    }
};

template <typename T>
struct send_impl_dispatcher<T, Require<Array<T>>> { using type = send_array_impl<std::decay_t<T>>; };

template <typename T>
struct recv_array_impl {
    using out_type = T;

    template <typename OidMap>
    static istream& apply(istream& in, size_type, const OidMap& oids, out_type& out) {
        pg_array array_header;
        pg_array_dimension dim_header;

        read(in, array_header);

        if (array_header.dimensions_count > 1) {
            throw system_error(error::bad_array_dimension,
                "multiply dimension count is not supported: "
                 + std::to_string(array_header.dimensions_count));
        }

        using item_type = unwrap_type<typename out_type::value_type>;

        if (!accepts_oid<item_type>(oids, array_header.elemtype)) {
            throw system_error(error::oid_type_mismatch,
                "unexpected oid " + std::to_string(array_header.elemtype)
                + " for element type of " + boost::core::demangle(typeid(item_type).name()));
        }

        if (array_header.dimensions_count < 1) {
            return in;
        }

        read(in, dim_header);

        if (dim_header.size == 0) {
            return in;
        }

        fit_array_size(out, dim_header.size);

        for (auto& item : out) {
            recv_data_frame(in, oids, item);
        }
        return in;
    }
};

template <typename T>
struct recv_impl_dispatcher<T, Require<Array<T>>> { using type = recv_array_impl<std::decay_t<T>>; };

} // namespace ozo::detail
