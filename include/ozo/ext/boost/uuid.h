#pragma once

#include <ozo/pg/definitions.h>
#include <ozo/io/send.h>
#include <ozo/io/recv.h>

#include <boost/uuid/uuid.hpp>

namespace ozo {

/**
 * @defgroup group-ext-boost-uuid boost::uuids::uuid
 * @ingroup group-ext-boost
 * @brief [boost::uuids::uuid](https://www.boost.org/doc/libs/1_62_0/libs/uuid/uuid.html) support
 *
 *@code
#include <ozo/ext/boost/uuid.h>
 *@endcode
 *
 * `boost::uuids::uuid` is defined as Universally Unique Identifierss data type.
 */

template <>
struct send_impl<boost::uuids::uuid> {
    template <typename M>
    static ostream& apply(ostream& out, const oid_map_t<M>&, const boost::uuids::uuid& in) {
        out.write(reinterpret_cast<const char*>(in.data), std::size(in));
        return out;
    }
};

template <>
struct recv_impl<boost::uuids::uuid> {
    template <typename M>
    static istream& apply(istream& in, size_type size, const oid_map_t<M>&, const boost::uuids::uuid& out) {
        in.read(const_cast<char*>(reinterpret_cast<const char*>(out.data)), size);
        return in;
    }
};

}

OZO_PG_BIND_TYPE(boost::uuids::uuid, "uuid")
