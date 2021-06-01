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
 * `boost::uuids::uuid` is defined as Universally Unique Identifiers data type.
 */

template <>
struct send_impl<boost::uuids::uuid> {
    template <typename OidMap>
    static ostream& apply(ostream& out, const OidMap&, const boost::uuids::uuid& in) {
        return write(out, in.data);
    }
};

template <>
struct recv_impl<boost::uuids::uuid> {
    template <typename OidMap>
    static istream& apply(istream& in, size_type, const OidMap&, boost::uuids::uuid& out) {
        return read(in, out.data);
    }
};

}

OZO_PG_BIND_TYPE(boost::uuids::uuid, "uuid")
