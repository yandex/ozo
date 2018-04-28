#pragma once

#include <ozo/detail/pg_type.h>

#include <libpq-fe.h>

#include <boost/hana/at_key.hpp>
#include <boost/hana/insert.hpp>
#include <boost/hana/string.hpp>
#include <boost/hana/map.hpp>
#include <boost/hana/pair.hpp>
#include <boost/hana/type.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/optional.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
// Fusion adaptors support
#include <boost/fusion/support/is_sequence.hpp>
#include <boost/fusion/include/is_sequence.hpp>

#include <ozo/optional.h>
#include <memory>
#include <string>
#include <list>
#include <vector>
#include <set>
#include <type_traits>

namespace ozo {

namespace hana = ::boost::hana;
using namespace hana::literals;

namespace fusion = ::boost::fusion;
/**
* PostgreSQL OID type - object identificator
*/
using oid_t = ::Oid;

/**
* Type for non initialized OID
*/
using null_oid_t = std::integral_constant<oid_t, 0>;
constexpr null_oid_t null_oid;

/**
* Indicates if type is nullable. By default - type is not
* nullable.
*/
template <typename T>
struct is_nullable : ::std::false_type {};

/**
* These next types are nullable
*/
template <typename T>
struct is_nullable<::boost::optional<T>> : ::std::true_type {};

#ifdef __OZO_STD_OPTIONAL
template <typename T>
struct is_nullable<__OZO_STD_OPTIONAL<T>> : ::std::true_type {};
#endif

template <typename T>
struct is_nullable<::boost::scoped_ptr<T>> : ::std::true_type {};
template <typename T, typename Deleter>
struct is_nullable<::std::unique_ptr<T, Deleter>> : ::std::true_type {};
template <typename T>
struct is_nullable<::boost::shared_ptr<T>> : ::std::true_type {};
template <typename T>
struct is_nullable<::std::shared_ptr<T>> : ::std::true_type {};
template <typename T>
struct is_nullable<::boost::weak_ptr<T>> : ::std::true_type {};
template <typename T>
struct is_nullable<::std::weak_ptr<T>> : ::std::true_type {};

template <typename T>
inline auto is_null(const T& v) noexcept ->
    std::enable_if_t<is_nullable<std::decay_t<T>>::value, bool> {
    return !v;
}

template <typename T>
inline auto is_null(const ::boost::weak_ptr<T>& v) noexcept {
    return is_null(v.lock());
}

template <typename T>
inline auto is_null(const ::std::weak_ptr<T>& v) noexcept {
    return is_null(v.lock());
}

template <typename T>
constexpr auto is_null(const T&) noexcept ->
    std::enable_if_t<!is_nullable<std::decay_t<T>>::value, std::false_type> {
    return {};
}

/**
* Indicates if type is an array
*/
template <typename T>
struct is_array : ::std::false_type {};

template <typename T, typename Alloc>
struct is_array<::std::vector<T, Alloc>> : ::std::true_type {};
template <typename T, typename Alloc>
struct is_array<::std::list<T, Alloc>> : ::std::true_type {};

/**
* Helper indicates if type is string
*/
template <typename T>
struct is_string : std::false_type {};

template <typename C, typename T, typename A>
struct is_string<std::basic_string<C, T, A>> : std::true_type {};

/**
* Indicates if type is adapted for introspection with Boost.Fusion
*/
template <typename T>
using is_fusion_adapted = std::integral_constant<bool,
    ::boost::fusion::traits::is_sequence<T>::value
>;

/**
* Indicates if type is adapted for introspection with Boost.Hana
*/
template <typename T>
using is_hana_adapted = std::integral_constant<bool,
    ::boost::hana::Sequence<T>::value ||
    ::boost::hana::Struct<T>::value
>;

/**
* Indicates if type is a composite. In general we suppose that composite
* is a type being adapted for introspection via Boost.Fusion or Boost.Hana,
* including tuples and compile-time sequences.
*/
template <typename T>
struct is_composite : std::integral_constant<bool,
    is_hana_adapted<T>::value ||
    is_fusion_adapted<T>::value
> {};


/**
* Type traits template forward declaration. Type traits contains information
* related to it's representation in the database. There are two different
* kind of traits - built-in types with constant OIDs and custom types with
* database depent OIDs. The functions below describe neccesary traits.
* For built-in types traits will be defined there. For custom types user
* must define traits.
*/
template <typename T>
struct type_traits;

/**
* Helpers to make size trait constant
*     bytes - makes fixed size trait
*     dynamic_size - makes dynamis size trait
*/
template <std::size_t n>
using bytes = std::integral_constant<std::size_t, n>;
using dynamic_size = void;

/**
* Helper defines the way for the type traits definitions.
* Type is undefined then Name type is not defined.
*     Name - type which can be converted into a string representation
*         which contain the fully qualified type name in DB
*     Oid - oid type - provides type's oid in database, may be defined for
*         built-in types only; custom types have only dynamic oid, depended
*         from the current state of DB.
*     Size - size type - provides information about type's object size, may
*         be specified only if the type's object has fixed size.
*/
template <typename Name, typename Oid = null_oid_t, typename Size = dynamic_size>
struct type_traits_helper {
    using name = Name;
    using oid = Oid;
    using size = Size;
};

/**
* By default all non specialized types have unspecified name, name
* oid and size.
*/
template <typename T>
struct type_traits : type_traits_helper<void> {};

/**
* Condition indicates if the specified type is built-in for PG
*/
template <typename T>
struct is_built_in : std::integral_constant<bool,
    !std::is_same_v<typename type_traits<T>::oid, null_oid_t>> {};

template <typename T>
using is_dynamic_size = std::is_same<typename type_traits<T>::size, dynamic_size>;

/**
* Function returns type name in Postgre SQL. For custom types user must
* define this name.
*/
template <typename T>
constexpr auto type_name(const T&) noexcept {
    constexpr auto name = typename type_traits<std::decay_t<T>>::name{};
    constexpr char const* retval = hana::to<char const*>(name);
    return retval;
}

/**
* Function returns object size.
*/
template <typename T>
constexpr auto size_of(T&&) noexcept  -> typename std::enable_if<
        !is_dynamic_size<std::decay_t<T>>::value,
        typename type_traits<std::decay_t<T>>::size>::type {
    return {};
}

template <typename T>
constexpr auto size_of(const T& v) noexcept -> typename std::enable_if<
        is_dynamic_size<std::decay_t<T>>::value,
        decltype(std::size(std::declval<T>()))>::type {
    return std::size(v) ? std::size(v) * size_of(*std::begin(v)) : 0;
}

} // namespace ozo

#define OZO__TYPE_NAME_TYPE(Name) decltype(Name##_s)

#define OZO_PG_DEFINE_TYPE(Type, Name, Oid, Size) \
    namespace ozo {\
        template <>\
        struct type_traits<Type> : type_traits_helper<\
            OZO__TYPE_NAME_TYPE(Name), \
            std::integral_constant<oid_t, Oid>, \
            Size\
        >{};\
    }

#define OZO_PG_DEFINE_CUSTOM_TYPE(Type, Name, Size) \
    namespace ozo {\
        template <>\
        struct type_traits<Type> : type_traits_helper<\
            OZO__TYPE_NAME_TYPE(Name), \
            null_oid_t, \
            Size\
        >{};\
    }

OZO_PG_DEFINE_TYPE(bool, "bool", BOOLOID, bytes<1>)
OZO_PG_DEFINE_TYPE(char, "char", CHAROID, bytes<1>)
OZO_PG_DEFINE_TYPE(std::vector<char>, "bytea", BYTEAOID, dynamic_size)

OZO_PG_DEFINE_TYPE(boost::uuids::uuid, "uuid", UUIDOID, bytes<16>)
OZO_PG_DEFINE_TYPE(std::vector<boost::uuids::uuid>, "uuid[]", 2951, dynamic_size)

OZO_PG_DEFINE_TYPE(int64_t, "int8", INT8OID, bytes<8>)
OZO_PG_DEFINE_TYPE(std::vector<int64_t>, "int8[]", 1016, dynamic_size)
OZO_PG_DEFINE_TYPE(int32_t, "int4", INT4OID, bytes<4>)
OZO_PG_DEFINE_TYPE(std::vector<int32_t>, "int4[]", INT4ARRAYOID, dynamic_size)
OZO_PG_DEFINE_TYPE(int16_t, "int2", INT2OID, bytes<2>)
OZO_PG_DEFINE_TYPE(std::vector<int16_t>, "int2[]", INT2ARRAYOID, dynamic_size)

OZO_PG_DEFINE_TYPE(ozo::oid_t, "oid", OIDOID, bytes<4>)
OZO_PG_DEFINE_TYPE(std::vector<ozo::oid_t>, "oid[]", OIDARRAYOID, dynamic_size)

OZO_PG_DEFINE_TYPE(double, "float8", FLOAT8OID, bytes<8>)
OZO_PG_DEFINE_TYPE(std::vector<double>, "float8[]", 1022, dynamic_size)
OZO_PG_DEFINE_TYPE(float, "float4", FLOAT4OID, bytes<4>)
OZO_PG_DEFINE_TYPE(std::vector<float>, "float4[]", FLOAT4ARRAYOID, dynamic_size)

OZO_PG_DEFINE_TYPE(std::string, "text", TEXTOID, dynamic_size)
OZO_PG_DEFINE_TYPE(std::vector<std::string>, "text[]", TEXTARRAYOID, dynamic_size)


namespace ozo {

template <typename ImplT>
struct oid_map_t {
    using impl_type = ImplT;

    impl_type impl;
};

namespace detail {

template <typename ... T>
constexpr decltype(auto) register_types() noexcept {
    return hana::make_map(
        hana::make_pair(hana::type_c<T>, null_oid()) ...
    );
}

} // namespace detail

template <typename ... T>
constexpr decltype(auto) register_types() noexcept {
    using impl_type = decltype(detail::register_types<T ...>());
    return oid_map_t<impl_type> {detail::register_types<T ...>()};
}

using empty_oid_map = std::decay_t<decltype(register_types<>())>;

/**
* Function sets oid for type in oid map.
*/
template <typename T, typename MapImplT>
inline void set_type_oid(oid_map_t<MapImplT>& map, oid_t oid) noexcept {
    static_assert(!is_built_in<T>::value, "type must not be built-in");
    map.impl[hana::type_c<T>] = oid;
}

/**
* Function returns oid for type from oid map.
*/
template <typename T, typename MapImplT>
inline auto type_oid(const oid_map_t<MapImplT>& map) noexcept
        -> std::enable_if_t<!is_built_in<T>::value, oid_t> {
    return map.impl[hana::type_c<T>];
}

template <typename T, typename MapImplT>
constexpr auto type_oid(const oid_map_t<MapImplT>&) noexcept
        -> std::enable_if_t<is_built_in<T>::value, oid_t> {
    return typename type_traits<T>::oid();
}

template <typename T, typename MapImplT>
inline auto type_oid(const oid_map_t<MapImplT>& map, const T&) noexcept{
    return type_oid<std::decay_t<T>>(map);
}

/**
* Function returns true if type can be obtained from DB response with
* specified oid.
*/
template <typename T, typename MapImplT>
inline bool accepts_oid(const oid_map_t<MapImplT>& map, oid_t oid) noexcept {
    return type_oid<T>(map) == oid;
}

template <typename T, typename MapImplT>
inline bool accepts_oid(const oid_map_t<MapImplT>& map, const T&, oid_t oid) noexcept {
    return accepts_oid<std::decay_t<T>>(map, oid);
}

} // namespace ozo
