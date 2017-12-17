#pragma once

#include "apq/error_code.h"
#include "apq/type_traits.h"

#include <boost/endian/conversion.hpp>

namespace apq {

// TODO: remove these when we get the name right
using libapq::oid_t;
using libapq::accepts_oid;
using libapq::is_string;

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
        return success;
    }
};

template <typename T>
struct recv<T, typename std::enable_if_t<std::is_integral<T>::value>>
{
    inline error_code operator()(oid_t, const char* bytes, std::size_t size, T& value)
    {
        using boost::endian::big_to_native;
     
        if (size != sizeof(value)) {
            return size_mismatch;
        }

        const T* p_be = reinterpret_cast<const T*>(bytes);
        value = big_to_native(*p_be);
        return success;
    }
};

}

template <typename T, typename TypeMap>
error_code convert_value(oid_t oid, const char* bytes, std::size_t size, const TypeMap& type_map, T& value)
{
    if (!accepts_oid(type_map, value, oid)) {
        return type_mismatch;
    }

    return detail::recv<T>{}(oid, bytes, size, value);
}

}