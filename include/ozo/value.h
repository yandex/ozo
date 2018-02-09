#pragma once

#include "ozo/error.h"
#include "ozo/type_traits.h"

#include <boost/endian/conversion.hpp>

namespace ozo {

namespace detail {

template <typename T, typename Enable = void>
struct recv
{
    error_code operator()(oid_t oid, const char* bytes, std::size_t size, T& value);
};


template <typename T>
struct recv<T, typename std::enable_if_t<is_string<T>::value>>
{
    inline error_code operator()(oid_t, const char* bytes, std::size_t size, T& value)
    {
        // TODO: client to server encoding conversion?
        value.assign(bytes, bytes + size);
        return error::ok;
    }
};

template <typename T>
struct recv<T, typename std::enable_if_t<std::is_integral<T>::value>>
{
    inline error_code operator()(oid_t, const char* bytes, std::size_t size, T& value)
    {
        using boost::endian::big_to_native;

        if (size != sizeof(value)) {
            return error::integer_value_size_mismatch;
        }

        const T* p_be = reinterpret_cast<const T*>(bytes);
        value = big_to_native(*p_be);
        return error::ok;
    }
};

}

template <typename T, typename TypeMap>
error_code convert_value(oid_t oid, const char* bytes, std::size_t size, const TypeMap& type_map, T& value)
{
    if (!accepts_oid(ozo::oid_map_t<TypeMap> {type_map}, value, oid)) {
        return error::oid_type_mismatch;
    }

    return detail::recv<T>{}(oid, bytes, size, value);
}

}