#pragma once

#include <ozo/detail/pg_type.h>
#include <ozo/detail/float.h>

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
#include <boost/make_shared.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/serialization/strong_typedef.hpp>

// Fusion adaptors support
#include <boost/fusion/support/is_sequence.hpp>
#include <boost/fusion/include/is_sequence.hpp>

#include <ozo/concept.h>
#include <ozo/optional.h>
#include <memory>
#include <string>
#include <list>
#include <vector>
#include <set>
#include <type_traits>

/**
 * @defgroup group-type_system Type system
 * @ingroup group-core
 * @brief Database related type system of the library.
 */
namespace ozo {

namespace hana = boost::hana;
using namespace hana::literals;

namespace fusion = boost::fusion;
/**
 * @brief PostgreSQL OID type - object identifier
 * @ingroup group-type_system
 */
using oid_t = ::Oid;

/**
 * @brief Type for non initialized OID
 * @ingroup group-type_system
 */
using null_oid_t = std::integral_constant<oid_t, 0>;
constexpr null_oid_t null_oid;

template <typename T, typename Enable = void>
struct is_nullable : std::false_type {};

/**
* These next types are nullable
*/
template <typename T>
struct is_nullable<boost::optional<T>> : std::true_type {};

#ifdef __OZO_STD_OPTIONAL
template <typename T>
struct is_nullable<__OZO_STD_OPTIONAL<T>> : std::true_type {};
#endif

template <typename T>
struct is_nullable<boost::scoped_ptr<T>> : std::true_type {};
template <typename T, typename Deleter>
struct is_nullable<std::unique_ptr<T, Deleter>> : std::true_type {};
template <typename T>
struct is_nullable<boost::shared_ptr<T>> : std::true_type {};
template <typename T>
struct is_nullable<std::shared_ptr<T>> : std::true_type {};
template <typename T>
struct is_nullable<boost::weak_ptr<T>> : std::true_type {};
template <typename T>
struct is_nullable<std::weak_ptr<T>> : std::true_type {};

/**
 * @brief Indicates if type meets nullable requirements.
 *
 * `Nullble` type has to:
 * * have a null-state,
 * * be `bool` convertable - `false` indicates null state,
 * * be dereferenceable via `operator *`.
 *
 * These next types are `Nullable` out of the box:
 * * `boost::optional`,
 * * `std::optional`,
 * * `boost::scoped_ptr`,
 * * `std::unique_ptr`,
 * * `boost::shared_ptr`,
 * * `std::shared_ptr`,
 * * `boost::weak_ptr`,
 * * `std::weak_ptr`
 *
 * @note Raw pointers are not supported as `Nullable` by default. But if it is really needed you can add them as described below
 *
 * If you want to extend this list - you have to specialize `ozo::is_nullable` struct like this:
 *
 * @code
    template <typename T>
    struct is_nullable<std::weak_ptr<T>> : std::true_type {};
 * @endcode
 * @sa ozo::unwrap_nullable(), is_null(), allocate_nullable(), init_nullable(), reset_nullable()
 * @ingroup group-core-concepts
 * @hideinitializer
 */
template <typename T>
constexpr auto Nullable = is_nullable<std::decay_t<T>>::value;


#ifdef OZO_DOCUMENTATION
/**
 * @brief Dereference #Nullable argument or forward it
 *
 * This utility function gets value from #Nullable, and pass object as it is if it is not #Nullable
 *
 * @param value --- object to unwrap
 * @return dereferenced argument in case of #Nullable, pass as is if it is not.
 * @ingroup group-core-functions
 */
template <typename T>
decltype(auto) unwrap_nullable(T&& value) noexcept;
#else
template <typename T>
inline decltype(auto) unwrap_nullable(T&& t, Require<Nullable<T>>* = 0) noexcept {
    return *t;
}

template <typename T>
inline decltype(auto) unwrap_nullable(T&& t, Require<!Nullable<T>>* = 0) noexcept {
    return std::forward<T>(t);
}
#endif
template <typename T>
using unwrap_nullable_type = typename std::decay_t<decltype(unwrap_nullable(
        std::declval<T>()))>;

template <typename T>
inline auto is_null(const T& v) noexcept -> std::enable_if_t<Nullable<T>, bool> {
    return !v;
}

template <typename T>
inline auto is_null(const boost::weak_ptr<T>& v) noexcept {
    return is_null(v.lock());
}

template <typename T>
inline auto is_null(const std::weak_ptr<T>& v) noexcept {
    return is_null(v.lock());
}

#ifdef OZO_DOCUMENTATION
/**
 * @brief Indicates if value is in null state
 *
 * This utility function indicates if given value is in null state.
 * If argument is not #Nullable it always returns `false`
 *
 * @param value --- object to examine
 * @return `true` if object is in null-state, overwise - `false`.
 * @ingroup group-core-functions
 */
template <typename T>
constexpr auto is_null(const T&) noexcept;
#endif

template <typename T>
constexpr auto is_null(const T&) noexcept -> std::enable_if_t<!Nullable<T>, std::false_type> {
    return {};
}

template <typename T, typename = std::void_t<>>
struct allocate_nullable_impl {
    static_assert(Emplaceable<T>, "default implementation uses emplace() method");
    template <typename Alloc>
    static void apply(T& out, const Alloc&) {
        out.emplace();
    }
};

template <typename T>
struct allocate_nullable_impl<std::unique_ptr<T>> {
    template <typename Alloc>
    static void apply(std::unique_ptr<T>& out, const Alloc&) {
        out = std::make_unique<T>();
    }
};

template <typename T>
struct allocate_nullable_impl<boost::scoped_ptr<T>> {
    template <typename Alloc>
    static void apply(boost::scoped_ptr<T>& out, const Alloc&) {
        out.reset(new T{});
    }
};

template <typename T>
struct allocate_nullable_impl<boost::shared_ptr<T>> {
    template <typename Alloc>
    static void apply(boost::shared_ptr<T>& out, const Alloc& a) {
        out = boost::allocate_shared<T, Alloc>(a);
    }
};

template <typename T>
struct allocate_nullable_impl<std::shared_ptr<T>> {
    template <typename Alloc>
    static void apply(std::shared_ptr<T>& out, const Alloc& a) {
        out = std::allocate_shared<T, Alloc>(a);
    }
};

template <typename T, typename Alloc>
inline void allocate_nullable(T& out, const Alloc& a) {
    static_assert(Nullable<T>, "T must be nullable");
    allocate_nullable_impl<std::decay_t<T>>::apply(out, a);
}

template <typename T, typename Alloc = std::allocator<char>>
inline void init_nullable(T& n, const Alloc& a = Alloc{}) {
    static_assert(Nullable<T>, "T must be nullable");
    if (!n) {
        allocate_nullable(n, a);
    }
}

template <typename T>
inline void reset_nullable(T& n) {
    static_assert(Nullable<T>, "T must be nullable");
    n = T{};
}

/**
* Indicates if type is an array
*/
template <typename T>
struct is_array : std::false_type {};

template <typename T, typename Alloc>
struct is_array<std::vector<T, Alloc>> : std::true_type {};
template <typename T, typename Alloc>
struct is_array<std::list<T, Alloc>> : std::true_type {};

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
    boost::fusion::traits::is_sequence<T>::value
>;

/**
* Indicates if type is adapted for introspection with Boost.Hana
*/
template <typename T>
using is_hana_adapted = std::integral_constant<bool,
    hana::Sequence<T>::value ||
    hana::Struct<T>::value
>;

/**
 * @brief Indicates if type is a composite.
 * @ingroup group-type_system
 * In general we suppose that composite
 * is a type being adapted for introspection via Boost.Fusion or Boost.Hana,
 * including tuples and compile-time sequences.
 *
 * @return std::true_type for composite type or std::false_type in other case
 */
#ifdef OZO_DOCUMENTATION
template <typename T>
struct is_composite;
#else
template <typename T>
struct is_composite : std::integral_constant<bool,
    is_hana_adapted<T>::value ||
    is_fusion_adapted<T>::value
> {};
#endif

/**
 * @brief Type traits template forward declaration.
 * @ingroup group-type_system
 *
 * Type traits contains information
 * related to it's representation in the database. There are two different
 * kind of traits - built-in types with constant OIDs and custom types with
 * database depent OIDs. The functions below describe neccesary traits.
 * For built-in types traits will be defined there. For custom types user
 * must define traits.
 *
 * @tparam T --- type to examine
 */
#ifdef OZO_DOCUMENTATION
template <typename T>
struct type_traits {
    using name = ; //!< `boost::hana::string` with name of the type in a database
    using oid = ; //!< `std::integral_constant` with Oid of the built-in type or null_oid_t for non built-in type
    using size = ; //!< `std::integral_constant` with size of the type object in bytes or `ozo::dynamic_size` in other case
};
#else
template <typename T>
struct type_traits;
#endif
/**
* Helpers to make size trait constant
*     bytes - makes fixed size trait
*     dynamic_size - makes dynamis size trait
*/
template <std::size_t n>
using bytes = std::integral_constant<std::size_t, n>;
using dynamic_size = void;

template <typename T, typename Size>
struct type_size_match : std::integral_constant<bool, sizeof(T) == Size::value> {};

template <typename T>
struct type_size_match<T, dynamic_size> : std::true_type {};
/**
 * @brief Helper defines the way for the type traits definitions.
 * @ingroup group-type_system
 *
 * Type is undefined then Name type is not defined.
 * @tparam Name --- type which can be converted into a string representation
 * which contain the fully qualified type name in DB
 * @tparam Oid --- oid type - provides type's oid in database, may be defined for
 * built-in types only; custom types have only dynamic oid, depended
 * from the current state of DB.
 * @tparam Size --- size type - provides information about type's object size, may
 *         be specified only if the type's object has fixed size.
 */
template <typename T, typename Name, typename Oid = null_oid_t, typename Size = dynamic_size>
struct type_traits_helper {
    using name = Name;
    using oid = Oid;
    using size = Size;
    static_assert(type_size_match<T, size>::value,
        "type size does not match to declared size");
};

template <typename T>
struct type_traits : type_traits_helper<T, void> {};

/**
 * @brief Condition indicates if the specified type is built-in for PG
 * @ingroup group-type_system
 * @tparam T -- type to check
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
constexpr auto type_name() noexcept {
    static_assert(!std::is_void<typename type_traits<T>::name>::value,
        "no type_traits found for the type");
    constexpr auto name = typename type_traits<T>::name{};
    constexpr char const* retval = hana::to<char const*>(name);
    return retval;
}

template <typename T>
constexpr auto type_name(const T&) noexcept {return type_name<T>();}

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

namespace pg {

BOOST_STRONG_TYPEDEF(std::string, name)

} // namespace pg
} // namespace ozo

#define OZO__TYPE_NAME_TYPE(Name) decltype(Name##_s)
#define OZO__TYPE_ARRAY_NAME_TYPE(Name) decltype(Name##_s+"[]"_s)

#define OZO_PG_DEFINE_TYPE(Type, Name, Oid, Size) \
    namespace ozo {\
        template <>\
        struct type_traits<Type> : type_traits_helper<\
            Type, \
            OZO__TYPE_NAME_TYPE(Name), \
            std::integral_constant<oid_t, Oid>, \
            Size\
        >{};\
    }

#define OZO_PG_DEFINE_TYPE_ARRAY(Type, Name, Oid) \
    namespace ozo {\
        template <>\
        struct type_traits<std::vector<Type>> : type_traits_helper<\
            std::vector<Type>, \
            OZO__TYPE_ARRAY_NAME_TYPE(Name), \
            std::integral_constant<oid_t, Oid>, \
            dynamic_size\
        >{};\
    }

#ifdef __OZO_STD_OPTIONAL
#define OZO_PG_DEFINE_TYPE_ARRAY_OZO_OPTIONAL(Type, Name, Oid) \
    OZO_PG_DEFINE_TYPE_ARRAY(__OZO_STD_OPTIONAL<Type>, Name, Oid)
#else
#define OZO_PG_DEFINE_TYPE_ARRAY_OZO_OPTIONAL(Type, Name, Oid)
#endif

#define OZO_PG_DEFINE_TYPE_ARRAY_NULLABLES(Type, Name, Oid) \
    OZO_PG_DEFINE_TYPE_ARRAY(boost::optional<Type>, Name, Oid) \
    OZO_PG_DEFINE_TYPE_ARRAY_OZO_OPTIONAL(Type, Name, Oid) \
    OZO_PG_DEFINE_TYPE_ARRAY(boost::scoped_ptr<Type>, Name, Oid) \
    OZO_PG_DEFINE_TYPE_ARRAY(std::unique_ptr<Type>, Name, Oid) \
    OZO_PG_DEFINE_TYPE_ARRAY(boost::shared_ptr<Type>, Name, Oid) \
    OZO_PG_DEFINE_TYPE_ARRAY(std::shared_ptr<Type>, Name, Oid)

/**
 * @brief Helper macro to define type mapping
 * @ingroup group-type_system
 * In general type mapping is provided via `ozo::type_traits` specialization.
 * But for a single type you need to define a type, an array of the type, an
 * optional of the type (to support null), shared_ptr... and many other boilerplate.
 * To reduce such things this macro is made.
 *
 * @note This macro can be called in the global namespace only
 *
 * E.g. the definition of the `uuid` type looks like this:
@code
OZO_PG_DEFINE_TYPE_AND_ARRAY(boost::uuids::uuid, "uuid", UUIDOID, 2951, bytes<16>)
@endcode
 *
 * @param Type --- C++ type to be mapped to database type
 * @param Name --- string with name of database type
 * @param Oid --- oid for built-in type and `ozo::null_oid_t` for custom type
 * @param ArrayOid --- oid for an array of built-in type and `ozo::null_oid_t` for custom type
 * @param Size --- `bytes<N>` for fixed-size type (like integer, bigint and so on),
 * there N - size of the type in database, `dynamic_type` for dynamic size types (like `text`
 * `bytea` and so on)
 */
#ifdef OZO_DOCUMENTATION
#define OZO_PG_DEFINE_TYPE_AND_ARRAY(Type, Name, Oid, ArrayOid, Size)
#else
#define OZO_PG_DEFINE_TYPE_AND_ARRAY(Type, Name, Oid, ArrayOid, Size) \
    OZO_PG_DEFINE_TYPE(Type, Name, Oid, Size) \
    OZO_PG_DEFINE_TYPE_ARRAY(Type, Name, ArrayOid) \
    OZO_PG_DEFINE_TYPE_ARRAY_NULLABLES(Type, Name, ArrayOid)
#endif


#define OZO_PG_DEFINE_CUSTOM_TYPE(Type, Name, Size) \
    namespace ozo {\
        template <>\
        struct type_traits<Type> : type_traits_helper<\
            Type, \
            OZO__TYPE_NAME_TYPE(Name), \
            null_oid_t, \
            Size\
        >{};\
    }

OZO_PG_DEFINE_TYPE(bool, "bool", BOOLOID, bytes<1>)
OZO_PG_DEFINE_TYPE(char, "char", CHAROID, bytes<1>)
OZO_PG_DEFINE_TYPE(std::vector<char>, "bytea", BYTEAOID, dynamic_size)

OZO_PG_DEFINE_TYPE_AND_ARRAY(boost::uuids::uuid, "uuid", UUIDOID, 2951, bytes<16>)

OZO_PG_DEFINE_TYPE_AND_ARRAY(int64_t, "int8", INT8OID, 1016, bytes<8>)
OZO_PG_DEFINE_TYPE_AND_ARRAY(int32_t, "int4", INT4OID, INT4ARRAYOID, bytes<4>)
OZO_PG_DEFINE_TYPE_AND_ARRAY(int16_t, "int2", INT2OID, INT2ARRAYOID, bytes<2>)

OZO_PG_DEFINE_TYPE_AND_ARRAY(ozo::oid_t, "oid", OIDOID, OIDARRAYOID, bytes<4>)

OZO_PG_DEFINE_TYPE_AND_ARRAY(double, "float8", FLOAT8OID, 1022, decltype(size_of(detail::to_integral(double()))))
OZO_PG_DEFINE_TYPE_AND_ARRAY(float, "float4", FLOAT4OID, FLOAT4ARRAYOID, decltype(size_of(detail::to_integral(float()))))

OZO_PG_DEFINE_TYPE_AND_ARRAY(std::string, "text", TEXTOID, TEXTARRAYOID, dynamic_size)

OZO_PG_DEFINE_TYPE_AND_ARRAY(ozo::pg::name, "name", NAMEOID, 1003, dynamic_size)

namespace ozo {

template <typename ImplT>
struct oid_map_t {
    using impl_type = ImplT;

    impl_type impl;
};


template <typename T>
struct is_oid_map : std::false_type {};

template <typename T>
struct is_oid_map<oid_map_t<T>> : std::true_type {};

template <typename T>
constexpr auto OidMap = is_oid_map<std::decay_t<T>>::value;

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

template <typename MapImplT>
inline constexpr bool empty(const oid_map_t<MapImplT>& map) noexcept {
    return hana::length(map.impl) == hana::size_c<0>;
}

} // namespace ozo
