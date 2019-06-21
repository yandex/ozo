#pragma once

#include <ozo/core/concept.h>
#include <ozo/type_traits.h>
#include <ozo/io/size_of.h>
#include <ozo/io/ostream.h>
#include <ozo/io/type_traits.h>
#include <libpq-fe.h>
#include <type_traits>

namespace ozo {

/**
 * @brief Defines how to send an object to an output stream.
 * @ingroup group-io-types
 *
 * This functor is used to serialize object as query parameter.
 *
 * @tparam In --- type of an object to apply to
 * @tparam <anonymous> --- SFINAE-based overloading parameter.
 *
 * The default implementation uses `ozo::write` function to serialize
 * simple objects like integers or strings.
 * To serialize complex types like #Array or #Composite special
 * internal implementations are used.
 *
 * @note This functor requires type definition via #OZO_PG_BIND_TYPE or
 * #OZO_PG_DEFINE_CUSTOM_TYPE.
 *
 * ### Customization point
 *
 * This template is a customization point for specializing serialization for user
 * defined types if it can not be obtained via the library. Typically user does not
 * need it.
 */
template <typename In, typename = std::void_t<>>
struct send_impl{
    static_assert(HasDefinition<In>, "type In must be defined as PostgreSQL type");

    static_assert(
        Readable<In&>,
        "In object type can't be sent. Probably it is not an arithmetic or doens't have data and size methods"
        " or it is a struct with field which can't be sent."
    );

    /**
     * @brief Implementation of serialization object into stream.
     *
     * @param out --- output stream
     * @param oid_map_t<M> --- #OidMap to get oid for custom types
     * @param in --- object to serialize
     * @return ostream& --- output stream
     */
    template <typename M>
    static ostream& apply(ostream& out, const oid_map_t<M>&, const In& in) {
        return write(out, in);
    }
};

namespace detail {

template <typename T, typename = std::void_t<>>
struct send_impl_dispatcher { using type = send_impl<std::decay_t<T>>; };

template <typename T, typename Tag>
struct send_impl_dispatcher<strong_typedef_wrapper<T, Tag>> { using type = send_impl<std::decay_t<T>>; };

template <typename T>
using get_send_impl = typename send_impl_dispatcher<unwrap_type<T>>::type;

} // namespace detail

/**
 * @brief Send object to an output stream.
 * @ingroup group-io-functions
 *
 * This function is used to serialize object as query parameter.
 * This function uses `ozo::send_impl` template specialization for simple types
 * like integers, floating points, strings, uuid and so on. This behaviour may be customized
 * via `ozo::send_impl` specialization.
 * To serialize complex types like #Array or #Composite types special internal implementations are used.
 * These implementation can not be customized.
 * This function unwraps type via `ozo::unwrap()` thus it handles properly all nullables
 * and wrapped types. If argument is #Nullable in null state the function does nothing. In
 * the other case it calls `ozo::send_impl::apply()` method for unwrapped object.
 *
 * @param out --- output stream.
 * @param oid_map --- #OidMap to determine object's oid.
 * @param in --- object to send.
 * @return ostream& --- reference to the output stream.
 */
template <class M, class In>
inline ostream& send(ostream& out, const oid_map_t<M>& oid_map, const In& in) {
    return is_null(in) ? out : detail::get_send_impl<In>::apply(out, oid_map, unwrap(in));
}

/**
 * @brief Send data frame of an object to an output stream.
 * @ingroup group-io-functions
 *
 * This function is used to write into stream object's data frame. E.g. it is used
 * for array items serialization. Data frame contains object's size and object's data.
 * See `ozo::data_frame_size()` for more details about data frame.
 *
 * @param out --- output stream
 * @param oid_map --- #OidMap to determine object's oid
 * @param in --- object to send
 * @return ostream& --- reference to the output stream
 */
template <class M, class In>
inline ostream& send_data_frame(ostream& out, const oid_map_t<M>& oid_map, const In& in) {
    write(out, size_of(in));
    return send(out, oid_map, in);
}

/**
 * @brief Send full frame of an object to an output stream.
 * @ingroup group-io-functions
 *
 * This function is used to write into stream object's frame. E.g. it is used
 * for composite items serialization. Frame contains object's type oid,
 * object's size and object's data.
 * See `ozo::frame_size()` for more details about the frame.
 *
 * @param out --- output stream
 * @param oid_map --- #OidMap to determine object's oid
 * @param in --- object to send
 * @return ostream& --- reference to the output stream
 */
template <class M, class In>
inline ostream& send_frame(ostream& out, const oid_map_t<M>& oid_map, const In& in) {
    write(out, type_oid(oid_map, in));
    return send_data_frame(out, oid_map, in);
}

} // namespace ozo
