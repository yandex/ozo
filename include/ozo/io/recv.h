#pragma once

#include <ozo/result.h>
#include <ozo/error.h>
#include <ozo/type_traits.h>
#include <ozo/io/size_of.h>
#include <ozo/core/concept.h>
#include <ozo/detail/endian.h>
#include <ozo/detail/float.h>
#include <ozo/io/istream.h>
#include <ozo/io/type_traits.h>
#include <boost/core/demangle.hpp>
#include <boost/hana/for_each.hpp>
#include <boost/hana/members.hpp>
#include <boost/hana/size.hpp>

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

/**
 * @brief Defines how to receive an object from an input stream.
 * @ingroup group-io-types
 *
 * This functor is used to deserialize object as query result.
 *
 * @tparam Out --- type of an object to apply to
 * @tparam <anonymous> --- SFINAE-based overloading parameter.
 *
 * The default implementation uses `ozo::read` function to deserialize
 * simple objects like integers or strings. For the #DynamicSize objects
 * method `resize()` will be called. So if your special implementation
 * of #DynamicSize type object does not have such method, you need to specialize
 * this template for the type to resize it to fit the incoming size (see the example below).
 * For the #StaticSize objects size check will be made. In case of incoming size does not
 * equal to value returned by `ozo::size_of()` for the object --- `std::range_error`
 * will be thrown.
 *
 * To deserialize complex types like #Array or #Composite special
 * internal implementations are used.
 *
 * @note This functor requires type definition via #OZO_PG_BIND_TYPE or
 * #OZO_PG_DEFINE_CUSTOM_TYPE.
 *
 * ### Customization point
 *
 * This template is a customization point for specializing deserialization for user
 * defined types if it can not be obtained via the library. Typically user does not
 * need it.
 *
 * ### Example
 *
 * @includedoc recv_impl_example.dox
 */
template <typename Out, typename = std::void_t<>>
struct recv_impl {
    static_assert(HasDefinition<Out>, "type Out must be defined as PostgreSQL type");

    /**
     * @brief Implementation of deserialization object from a stream.
     *
     * @param in --- input stream
     * @param size --- size of incoming data
     * @param OidMap --- #OidMap to get oid for custom types
     * @param out --- object to deserialize
     * @return ostream& --- input stream
     */
    template <typename OidMap>
    static istream& apply(istream& in, size_type size, const OidMap&, Out& out) {
        auto& real_out = [&] {
            if constexpr (StrongTypedef<Out>) {
                return std::ref(static_cast<typename Out::base_type&>(out));
            } else {
                return std::ref(static_cast<Out&>(out));
            }
        }().get();

        static_assert(
            Writable<decltype(real_out)> || !Readable<decltype(real_out)>,
            "Out type object can't be received. Probably it is read only type"
            " or it is an adapted struct with read only field."
        );

        static_assert(
            Writable<decltype(real_out)>,
            "Out type object can't be received. Probably it is not an arithmetic"
            " or doesn't have non const data and size methods "
            " or it is a struct with field which can't be received."
        );

        if constexpr (DynamicSize<Out>) {
            static_assert(Resizable<decltype(real_out)>,
                "Out type object has dynamic size but doesn't have resize method."
            );
            real_out.resize(size);
        } else if (size != size_of(real_out)) {
            throw ozo::system_error(error::bad_object_size,
                "data size " + std::to_string(size)
                + " does not match type size " + std::to_string(size_of(real_out)));
        }
        return read(in, real_out);
    }
};

namespace detail {

template <typename T, typename = std::void_t<>>
struct recv_impl_dispatcher { using type = recv_impl<std::decay_t<T>>; };

template <typename T>
using get_recv_impl = typename recv_impl_dispatcher<unwrap_type<T>>::type;

template <typename OidMap, typename Oid, typename Out>
inline istream& recv(istream& in, [[maybe_unused]] Oid oid, size_type size, const OidMap& oids, Out& out) {
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

    if constexpr (Nullable<Out>) {
        init_nullable(out);
    } else if (size == null_state_size) {
        throw std::invalid_argument("unexpected null for type "
            + boost::core::demangle(typeid(out).name()));
    }

    return detail::get_recv_impl<Out>::apply(in, size, oids, ozo::unwrap(out));
}

template <typename OidMap, typename Oid, typename Out>
inline istream& recv_data_frame(istream& in, Oid oid, const OidMap& oids, Out& out) {
    size_type size = 0;
    read(in, size);
    return recv(in, oid, size, oids, out);
}

} // namespace detail

/**
 * @brief Receive object from an input stream and control incoming oid
 * @ingroup group-io-functions
 *
 * This function is used to get an object from an input stream.
 * It checks:
 * * incoming oid is acceptable by output object type --- if oid can not be accepted by the type
 *   `ozo::system_error` with `ozo::error::oid_type_mismatch` will be thrown, it uses `ozo::accepts_oid()`
 *   for this check;
 * * null state is acceptable --- if output object is not #Nullable, but null state received then
 *   `std::invalid_argument` will be thrown.
 *
 * For nullables the function allocates or resets nullables according to incoming data size which
 * indicates null state via `ozo::null_state_size`. `ozo::reset_nullable()` and `ozo::init_nullable()`
 * are used.
 *
 * For deserialization implementation of simple objects like strings, integers, uuid and so on
 * `ozo::recv_impl` is used. To deserialize complex types like #Array or #Composite special
 * internal implementations are used.
 *
 * @param in --- input stream to read data from.
 * @param oid --- incoming oid type to check.
 * @param size --- incoming data size.
 * @param oids --- #OidMap to get oid for custom types from
 * @param out --- object to deserialize into
 * @return istream& --- input stream
 */
template <typename OidMap, typename Out>
inline istream& recv(istream& in, oid_t oid, size_type size, const OidMap& oids, Out& out) {
    return detail::recv(in, oid, size, oids, out);
}

/**
 * @brief Receive data frame of an object from an input stream.
 * @ingroup group-io-functions
 *
 * This function is used to read from stream object's data frame to receive the object.
 * E.g. it is used for array items deserialization. Data frame contains object's
 * size and object's data.
 * See `ozo::data_frame_size()` for more details about data frame.
 *
 * @note This function does not check objects oid on purpose.
 *
 * @param in --- input stream
 * @param oids --- #OidMap to determine possible nested object's oid
 * @param out --- object to receive
 * @return ostream& --- reference to the input stream
 */
template <typename OidMap, typename Out>
inline istream& recv_data_frame(istream& in, const OidMap& oids, Out& out) {
    return detail::recv_data_frame(in, null_oid, oids, out);
}

/**
 * @brief Receive full frame of an object from an input stream.
 * @ingroup group-io-functions
 *
 * This function is used to read from stream object's frame and receive the object.
 * E.g. it is used for composite items deserialization. Frame contains object's type oid,
 * object's size and object's data. See `ozo::frame_size()` for more details
 * about the frame.
 *
 * @note The function uses `ozo::recv` underhood, so all the checks will be made.
 *
 * @param in --- input stream
 * @param oids --- #OidMap to determine object's oid
 * @param out --- object to receive
 * @return ostream& --- reference to the output stream
 */
template <typename OidMap, typename Out>
inline istream& recv_frame(istream& in, const OidMap& oids, Out& out) {
    oid_t oid = null_oid;
    read(in, oid);
    return detail::recv_data_frame(in, oid, oids, out);
}

template <typename T, typename OidMap, typename Out>
void recv(const value<T>& in, const OidMap& oids, Out& out) {
    istream s(in.data(), in.size());
    recv(s, in.oid(), (in.is_null() ? null_state_size : in.size()), oids, out);
}

template <typename T, typename OidMap, typename Out>
Require<!FusionSequence<Out> && !FusionAdaptedStruct<Out> && !HanaStruct<Out>>
recv_row(const row<T>& in, const OidMap& oid_map, Out& out) {
    if (std::size(in) != 1) {
        throw std::range_error("row size " + std::to_string(std::size(in))
            + " does not equal 1 for single column result");
    }

    recv(*(in.begin()), oid_map, out);
}

template <typename T, typename OidMap, typename Out>
Require<FusionSequence<Out> && !FusionAdaptedStruct<Out> && !HanaStruct<Out>>
recv_row(const row<T>& in, const OidMap& oid_map, Out& out) {

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

template <typename T, typename OidMap, typename Out>
Require<FusionAdaptedStruct<Out> && !HanaStruct<Out>>
recv_row(const row<T>& in, const OidMap& oid_map, Out& out) {

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

template <typename T, typename OidMap, typename Out>
Require<HanaStruct<Out>>
recv_row(const row<T>& in, const OidMap& oid_map, Out& out) {

    const auto keys = hana::keys(out);
    const auto size = size_type(hana::value(hana::size(keys)));
    if (size != std::size(in)) {
        throw std::range_error("row size " + std::to_string(std::size(in))
            + " does not match structure " + boost::core::demangle(typeid(out).name())
            + " size " + std::to_string(size));
    }

    hana::for_each(keys, [&](auto key) {
        auto i = in.find(hana::to<const char*>(key));
        if (i == in.end()) {
            throw std::range_error(std::string("row does not contain \"")
                + hana::to<const char*>(key) + "\" column for "
                + boost::core::demangle(typeid(out).name()));
        } else {
            recv(*i, oid_map, hana::at_key(out, key));
        }
    });
}

template <typename T, typename OidMap, typename Out>
Require<ForwardIterator<Out>, Out>
recv_result(const basic_result<T>& in, const OidMap& oid_map, Out out) {
    for (auto row : in) {
        recv_row(row, oid_map, *out++);
    }
    return out;
}

template <typename T, typename OidMap, typename Out>
Require<InsertIterator<Out>, Out>
recv_result(const basic_result<T>& in, const OidMap& oid_map, Out out) {
    for (auto row : in) {
        typename Out::container_type::value_type v{};
        recv_row(row, oid_map, v);
        *out++ = std::move(v);
    }
    return out;
}

template <typename T, typename OidMap>
basic_result<T>& recv_result(basic_result<T>& in, const OidMap&, basic_result<T>& out) {
    out = std::move(in);
    return out;
}

template <typename T, typename OidMap, typename Out>
basic_result<T>& recv_result(basic_result<T>& in, const OidMap& oid_map, std::reference_wrapper<Out> out) {
    return recv_result(in, oid_map, out.get());
}

} // namespace ozo
