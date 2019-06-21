#pragma once

#include <ozo/detail/pg_type.h>
#include <ozo/detail/float.h>
#include <ozo/core/strong_typedef.h>
#include <ozo/core/nullable.h>
#include <ozo/core/unwrap.h>
#include <ozo/core/concept.h>
#include <ozo/optional.h>

#include <libpq-fe.h>

#include <boost/hana/at_key.hpp>
#include <boost/hana/insert.hpp>
#include <boost/hana/string.hpp>
#include <boost/hana/map.hpp>
#include <boost/hana/pair.hpp>
#include <boost/hana/type.hpp>

#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <type_traits>

/**
 * @defgroup group-type_system Type system
 * @brief Database-related type system of the library.
 */

/**
 * @defgroup group-type_system-types Types
 * @ingroup group-type_system
 * @brief Database-related type system types.
 */

/**
 * @defgroup group-type_system-concepts Concepts
 * @ingroup group-type_system
 * @brief Database-related type system concepts.
 */

/**
 * @defgroup group-type_system-constants Constants
 * @ingroup group-type_system
 * @brief Database-related type system constants.
 */

/**
 * @defgroup group-type_system-functions Functions
 * @ingroup group-type_system
 * @brief Database-related type system functions.
 */

/**
 * @defgroup group-type_system-mapping Mapping
 * @ingroup group-type_system
 * @brief Types mapping C++ to PostgreSQL
 */
namespace ozo {

namespace hana = boost::hana;
using namespace hana::literals;

namespace fusion = boost::fusion;
/**
 * @brief PostgreSQL OID type - object identifier
 * @ingroup group-type_system-types
 */
using oid_t = ::Oid;

/**
 * @brief OID constant type.
 *
 * This type is like `std::bool_constant` - used to enable type level
 * resolving of constants. All `BuiltIn` types uses this template
 * specialization for their OID constants.
 *
 * @ingroup group-type_system-types
 * @tparam Oid --- OID value
 */
template <oid_t Oid>
struct oid_constant : std::integral_constant<oid_t, Oid> {};

/**
 * @brief Type for non initialized OID
 * @ingroup group-type_system-types
 */
using null_oid_t = oid_constant<0>;

/**
 * @brief Constant for empty OID
 * @ingroup group-type_system-constants
 */
constexpr null_oid_t null_oid;

/**
 * @brief Indicates if type is PostgreSQL array representation.
 * @ingroup group-type_system-types
 * Representation means that you can obtain PostgreSQL array into the supported
 * container.
 * Array by default can be represented via next containers:
 *   * @ref group-ext-std-vector
 *   * @ref group-ext-std-list
 *   * @ref group-ext-std-array
 *
 * @tparam T --- type to examine.
 *
 * ### Customization point
 *
 * User can add representation of `Array` by defining specialization of
 * `ozo::is_array` for the desired container like this:
 *
 * @code
template <typename ...Ts>
struct is_array<std::vector<Ts...>> : std::true_type {};
 * @endcode
 * @hideinitializer
 */
template <typename T>
struct is_array : std::false_type {};

/**
 * @brief Indicates if type is PostgreSQL array representation.
 * @ingroup group-type_system-concepts
 *
 * Representation means that you can obtain PostgreSQL array into the type.
 * See `ozo::is_array` for the list of supported array representation.
 * @tparam T --- type to examine.
 *
 * @hideinitializer
 */
template <typename T>
constexpr auto Array = is_array<std::decay_t<T>>::value;

namespace definitions {
template <typename T>
struct type;

template <typename T>
struct array;
} // namespace definitions

namespace detail {

template <typename T, typename = std::void_t<>>
struct get_type_traits { using type = void; };

template <typename T>
struct get_type_traits<T, std::void_t<typename definitions::type<T>::name>> {
    using type = definitions::type<T>;
};

template <typename T>
struct get_type_traits<T, Require<Array<T>>> {
    using type = definitions::array<unwrap_type<typename T::value_type>>;
};

}
/**
 * @brief Type traits template forward declaration.
 * @ingroup group-type_system-types
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
    using name = <implementation defined>; //!< `boost::hana::string` with name of the type in a database
    using oid = <implementation defined>; //!< `ozo::oid_constant` with Oid of the built-in type or null_oid_t for non built-in type
    using size = <implementation defined>; //!< `ozo::size_constant` with size of the type object in bytes or `ozo::dynamic_size` in other case
};
#endif

template <typename T>
using type_traits = typename detail::get_type_traits<unwrap_type<T>>::type;

/**
 * @brief Condition indicates if type has corresponding type traits for PostgreSQL
 *
 * @tparam T --- type to examine
 * @ingroup group-core-concepts
 * @hideinitializer
 */
template <typename T>
constexpr auto HasDefinition = !std::is_void_v<type_traits<T>>;

template <typename T>
struct is_composite : std::bool_constant<
    HanaStruct<T> || FusionAdaptedStruct<T>
> {};

/**
 * @brief Indicates if type is PostgreSQL composite representation.
 * @ingroup group-type_system-concepts
 *
 * Representation means that you can obtain PostgreSQL composite into the type.
 * The library treats as `Composite` Boost.Hana or Boost.Fusion adapted structures or
 * `std::tuple` as record.
 *
 * @tparam T --- type to examine.
 *
 * @hideinitializer
 */
template <typename T>
constexpr auto Composite = is_composite<std::decay_t<T>>::value;

/**
 * @brief PostgreSQL size type
 * @ingroup group-type_system-types
 */
using size_type = std::int32_t;

/**
 * @brief Object size constant type
 *
 * This type is like `std::bool_constant` - used to enable type level
 * resolving of constants. All `BuiltIn` types uses this template
 * specialization for their size constants.
 *
 * @tparam n --- size in bytes
 * @ingroup group-type_system-types
 */
template <size_type n>
struct size_constant : std::integral_constant<size_type, n> {};

constexpr size_constant<-1> null_state_size;

/**
* Helpers to make size trait constant
*     bytes - makes fixed size trait
*     dynamic_size - makes dynamic size trait
*/
template <size_type n>
using bytes = size_constant<n>;
struct dynamic_size : size_constant<-1> {};

template <typename T, typename = std::void_t<>>
struct is_built_in : std::false_type {};

template <typename T>
struct is_built_in<T, std::enable_if_t<
    !std::is_void_v<typename type_traits<T>::oid>>
> : std::true_type {};

/**
 * @brief Condition indicates if the specified type is built-in for PG
 * @ingroup group-type_system-concepts
 * @tparam T --- type to check
 * @hideinitializer
 */
template <typename T>
constexpr bool BuiltIn = is_built_in<std::decay_t<T>>::value;

template <typename T>
struct is_dynamic_size : std::is_same<typename type_traits<T>::size, dynamic_size> {};

/**
 * @brief Condition indicates if the specified type is has dynamic size
 * @ingroup group-type_system-concepts
 * @tparam T --- type to check
 * @hideinitializer
 */
template <typename T>
constexpr auto DynamicSize = is_dynamic_size<std::decay_t<T>>::value;

/**
 * @brief Condition indicates if the specified type is has fixed size
 * @ingroup group-type_system-concepts
 * @tparam T --- type to check
 * @hideinitializer
 */
template <typename T>
constexpr auto StaticSize = !DynamicSize<T>;

/**
* @brief Function returns type name in Postgre SQL.
* @tparam T --- type
* @return `const char*` --- name of the type
* @ingroup group-type_system-functions
*/
template <typename T>
constexpr auto type_name() noexcept {
    static_assert(!std::is_void_v<type_traits<T>>,
        "no type_traits found for the type");
    constexpr auto name = typename type_traits<T>::name{};
    constexpr char const* retval = hana::to<char const*>(name);
    return retval;
}

/**
* @brief Function returns type name in Postgre SQL.
* @param T --- type
* @return `const char*` --- name of the type
* @ingroup group-type_system-functions
*/
template <typename T>
constexpr auto type_name(const T&) noexcept {return type_name<T>();}

namespace detail {

template <typename Name, typename Oid = void, typename Size = dynamic_size>
struct type_definition {
    using name = Name;
    using oid = Oid;
    using size = Size;
};

template <typename Type, typename Oid>
using array_definition = type_definition<
    decltype(typename type_traits<Type>::name{} + "[]"_s),
    Oid
>;

} // namespace detail
} // namespace ozo


#define OZO_PG_DEFINE_TYPE_(Type, Name, OidType, Size) \
    namespace ozo::definitions {\
    template <>\
    struct type<Type> : detail::type_definition<decltype(Name##_s), OidType, Size>{\
        static_assert(std::is_same_v<Size, dynamic_size>\
            || Size::value == null_state_size\
            || static_cast<size_type>(sizeof(Type)) == Size::value,\
            #Type " type size does not match to declared size");\
    };\
    }

#define OZO_PG_DEFINE_TYPE(Type, Name, Oid, Size) \
    OZO_PG_DEFINE_TYPE_(Type, Name, oid_constant<Oid>, Size)

#define OZO_PG_DEFINE_TYPE_ARRAY_(Type, OidType) \
    namespace ozo::definitions {\
    template <>\
    struct array<Type> : detail::array_definition<Type, OidType>{};\
    }

#define OZO_PG_DEFINE_TYPE_AND_ARRAY_(Type, Name, OidType, ArrayOidType, Size) \
    OZO_PG_DEFINE_TYPE_(Type, Name, OidType, Size) \
    OZO_PG_DEFINE_TYPE_ARRAY_(Type, ArrayOidType)

/**
 * @brief Helper macro to define type mapping
 * @ingroup group-type_system-mapping
 * In general type mapping is provided via `ozo::definitions::type` and
 * `ozo::definitions::array` specialization.
 * To reduce the boilerplate code the macro exists.
 *
 * @note This macro can be called in the global namespace only
 *
 * @param Type --- C++ type to be mapped to database type
 * @param Name --- string with name of database type
 * @param Oid --- oid for built-in type and `void` for custom type
 * @param ArrayOid --- oid for an array of built-in type and `void` for custom type
 * @param Size --- `bytes<N>` for fixed-size type (like integer, bigint and so on),
 * there N - size of the type in database, `dynamic_type` for dynamic size types (like `text`
 * `bytea` and so on)
 *
 * ### Example
 *
 * E.g. a definition of `uuid` type looks like this:
@code
OZO_PG_DEFINE_TYPE_AND_ARRAY(boost::uuids::uuid, "uuid", UUIDOID, 2951, bytes<16>)
@endcode

@sa OZO_PG_DEFINE_CUSTOM_TYPE

 */
#ifdef OZO_DOCUMENTATION
#define OZO_PG_DEFINE_TYPE_AND_ARRAY(Type, Name, Oid, ArrayOid, Size)
#else
#define OZO_PG_DEFINE_TYPE_AND_ARRAY(Type, Name, Oid, ArrayOid, Size) \
    OZO_PG_DEFINE_TYPE_AND_ARRAY_(Type, Name, oid_constant<Oid>, oid_constant<ArrayOid>, Size)
#endif

/**
 * @brief Helper macro to define custom type mapping
 * @ingroup group-type_system-mapping
 * In general type mapping is provided via `ozo::definitions::type` and
 * `ozo::definitions::array` specialization.
 *
 * @note This macro can be called in the global namespace only
 *
 * @param Type --- C++ type to be mapped to database type
 * @param Name --- string with name of database type
 * @param Size --- optional parameter, `bytes<N>` for fixed-size type (like integer, bigint and so on),
 * there N - size of the type in database, `dynamic_type` for dynamic size types (like `text`
 * `bytea` and so on), if parameter not pointed size type will be evaluated basing on C++ type size.
 *
 * ### Example
 *
 * Definition of user defined composite type may look like this:
@code
BOOST_FUSION_DEFINE_STRUCT((smtp), message,
    (std::int64_t, id)
    (std::string, from)
    (std::vector<std::string>, to)
    (std::optional<std::string>, subject)
    (std::optional<std::string>, text)
);

//...

OZO_PG_DEFINE_CUSTOM_TYPE(smtp::message, "code.message")
@endcode

@sa OZO_PG_DEFINE_TYPE_AND_ARRAY
 */
#ifdef OZO_DOCUMENTATION
#define OZO_PG_DEFINE_CUSTOM_TYPE(Type, Name [, Size])
#else
#define OZO_PG_DEFINE_CUSTOM_TYPE_IMPL_3(Type, Name, Size) \
    OZO_PG_DEFINE_TYPE_AND_ARRAY_(Type, Name, void, void, Size)

#define OZO_PG_DEFINE_CUSTOM_TYPE_IMPL_2(Type, Name) \
    OZO_PG_DEFINE_CUSTOM_TYPE_IMPL_3(Type, Name, dynamic_size)

#define OZO_PG_DEFINE_CUSTOM_TYPE(...)\
    BOOST_PP_OVERLOAD(OZO_PG_DEFINE_CUSTOM_TYPE_IMPL_,__VA_ARGS__)(__VA_ARGS__)
#endif

namespace ozo {

/**
 * @brief OidMap implementation type.
 * @ingroup group-type_system-types
 *
 * This type implements OidMap concept based on `boost::hana::map`.
 */
template <typename ImplT>
struct oid_map_t {
    using impl_type = ImplT;

    impl_type impl;
};


template <typename T>
struct is_oid_map : std::false_type {};

template <typename T>
struct is_oid_map<oid_map_t<T>> : std::true_type {};

/**
 * @brief Map of C++ types to corresponding PostgreSQL types OIDs
 *
 * @ingroup group-type_system-concepts
 * `OidMap` is needed to store information about C++ types and corresponding
 * custom database types' OIDs. For PostgreSQL built-in types no mapping is needed since their
 * `OID`s are defined in PostgreSQL sources. For custom types their `OID`s are defined
 * in a database.
 *
 * ###OidMap Definition
 *
 * `OidMap` `map` is an object for which these next statements are valid:
 *
 @code
 oid_t oid;
 //...
 set_type_oid<T>(map, oid);
 @endcode
 * Sets oid for type T in the map.
 *
 @code
 oid_t oid = type_oid<T>(map);
 @endcode
 * Returns oid value for type T from the map.
 *
@code
bool res = empty(map);
@endcode
 * Returns true if map has no types OIDs.
 *
 * @hideinitializer
 * @sa oid_map_t, register_types(), set_type_oid(), type_oid(), accepts_oid()
 */
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

/**
 * @brief Provides #OidMap implementation for user-defined types.
 *
 * @ingroup group-type_system-functions
 * This function have to be used to provide information about custom types are being used
 * within requests for a #ConnectionSource.
 *
 * ###Example
 *
 * @code
// User defined type
struct custom_type;

//...

// Providing type information and corresponding database type
OZO_PG_DEFINE_TYPE_AND_ARRAY(custom_type, "code.custom_type", null_oid, null_oid, dynamic_size)

//...
// Creating ConnectionSource for futher requests to a database
const auto conn_source = ozo::make_connection_info("...", regiter_types<custom_type>());
 * @endcode
 *
 * @return `oid_map_t` object.
 */
template <typename ... T>
constexpr decltype(auto) register_types() noexcept {
    using impl_type = decltype(detail::register_types<T ...>());
    return oid_map_t<impl_type> {detail::register_types<T ...>()};
}

/**
 * @brief Type alias for empty #OidMap
 * @ingroup group-type_system-types
 */
using empty_oid_map = std::decay_t<decltype(register_types<>())>;

/**
* Function sets oid for a type in #OidMap.
* @ingroup group-type_system-functions
* @tparam T --- type to set oid for.
* @param map --- #OidMap to modify.
* @param oid --- OID to set.
*/
template <typename T, typename MapImplT>
inline void set_type_oid(oid_map_t<MapImplT>& map, oid_t oid) noexcept {
    static_assert(!is_built_in<T>::value, "type must not be built-in");
    map.impl[hana::type_c<T>] = oid;
}

/**
* @brief Function returns oid for a type from #OidMap.
* @ingroup group-type_system-functions
* @tparam T --- type to get OID for.
* @param map --- #OidMap to get OID from.
* @return oid_t --- OID of the type
*/
#ifdef OZO_DOCUMENTATION
template <typename T, typename OidMap>
oid_t type_oid(const OidMap& map) noexcept;
#else
template <typename T, typename MapImplT>
inline auto type_oid(const oid_map_t<MapImplT>& map) noexcept
        -> Require<!BuiltIn<T>, oid_t> {
    return map.impl[hana::type_c<unwrap_type<T>>];
}

template <typename T, typename MapImplT>
constexpr auto type_oid(const oid_map_t<MapImplT>&) noexcept
        -> Require<BuiltIn<T>, oid_t> {
    return typename type_traits<T>::oid();
}
#endif

/**
* @brief Function returns oid for type from #OidMap.
* @ingroup group-type_system-functions
* @param map --- #OidMap to get OID from.
* @param v --- object for which type's OID will return.
* @return oid_t --- OID of the type
*/
#ifdef OZO_DOCUMENTATION
template <typename T, typename OidMap>
oid_t type_oid(const OidMap& map, const T& v) noexcept
#else
template <typename T, typename MapImplT>
inline auto type_oid(const oid_map_t<MapImplT>& map, const T&) noexcept{
    return type_oid<std::decay_t<T>>(map);
}
#endif

/**
* Function returns true if type can be obtained from DB response with
* specified OID.
* @ingroup group-type_system-functions
* @tparam T --- type to examine
* @param map --- #OidMap to get type OID from
* @param oid --- OID to check for compatibility
*/
template <typename T, typename MapImplT>
inline bool accepts_oid(const oid_map_t<MapImplT>& map, oid_t oid) noexcept {
    return type_oid<T>(map) == oid;
}

/**
* Function returns true if type can be obtained from DB response with
* specified OID.
* @ingroup group-type_system-functions
* @param map --- #OidMap to get type OID from
* @param const T& --- type to examine
* @param oid --- OID to check for compatibility
*/
template <typename T, typename MapImplT>
inline bool accepts_oid(const oid_map_t<MapImplT>& map, const T& , oid_t oid) noexcept {
    return accepts_oid<std::decay_t<T>>(map, oid);
}

/**
 * Checks if #OidMap contains no items.
 * @ingroup group-type_system-functions
 *
 * ### Example
 *
 * @code
static_assert(empty(ozo::empty_oid_map{}));
 * @endcode
 * @param map --- #OidMap to check
 * @return `true` --- if map contains no items
 * @return `false` --- if contains items.
 */
template <typename MapImplT>
inline constexpr bool empty(const oid_map_t<MapImplT>& map) noexcept {
    return hana::length(map.impl) == hana::size_c<0>;
}

} // namespace ozo
