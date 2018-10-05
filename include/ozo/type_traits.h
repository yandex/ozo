#pragma once

#include <ozo/detail/pg_type.h>
#include <ozo/detail/float.h>
#include <ozo/detail/strong_typedef.h>

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

template <typename T, typename Enable = void>
struct is_nullable : std::false_type {};

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
template <>
struct is_nullable<std::nullptr_t> : std::true_type {};
template <>
struct is_nullable<__OZO_NULLOPT_T> : std::true_type {};

/**
 * @brief Indicates if type meets nullable requirements.
 *
 * `Nullable` type has to:
 * * have a null-state,
 * * be `bool` convertable - `false` indicates null state,
 * * be dereferenceable via `operator *`,
 * * have `allocate_nullable()` specialization (see the function documentation for how to).
 *
 * These next types are `Nullable` out of the box:
 * * `boost::optional`,
 * * `std::optional`,
 * * `boost::scoped_ptr`,
 * * `std::unique_ptr`,
 * * `boost::shared_ptr`,
 * * `std::shared_ptr`,
 * * `boost::weak_ptr`,
 * * `std::weak_ptr`.
 *
 * @note Raw pointers are not supported as `Nullable` by default, because this library uses RAII model for objects and
 * no special deallocation functions are used. But if it is really needed and you want to deallocate the allocated
 * objects manually you can add them as described below.
 *
 * ### Example
 *
 * If you want to add nullable type to the library --- you have to specialize `ozo::is_nullable` struct and specify allocation function like this:
 *
 * @code
//Add raw pointer to nullables

namespace ozo {

// Mark raw pointer as nullable
template <typename T>
struct is_nullable<T*> : std::true_type {};

// Specify allocation function
template <typename T>
struct allocate_nullable_impl<T*> {
    template <typename Alloc>
    static void apply(T*& out, const Alloc&) {
        out = new T();
    }
};

}

 * @endcode
 *
 * @sa ozo::unwrap(), is_null(), allocate_nullable(), init_nullable(), reset_nullable()
 * @ingroup group-core-concepts
 * @hideinitializer
 */
template <typename T>
constexpr auto Nullable = is_nullable<std::decay_t<T>>::value;

namespace detail {

struct unwrap_with_forward {
    template <typename T>
    constexpr static decltype(auto) apply(T&& v) noexcept {
        return std::forward<T>(v);
    }
};

struct unwrap_with_dereference {
    template <typename T>
    constexpr static decltype(auto) apply(T&& v) noexcept(noexcept(*v)) {
        return *v;
    }
};

struct unwrap_with_lock_dereference {
    template <typename T>
    constexpr static decltype(auto) apply(T&& v) noexcept(noexcept(*v.lock())) {
        return *v.lock();
    }
};

struct unwrap_with_get {
    template <typename T>
    constexpr static decltype(auto) apply(T&& v) noexcept(noexcept(v.get())) {
        return v.get();
    }
};

template <typename T, typename = std::void_t<>>
struct unwrap_impl_default : unwrap_with_forward {};

template <typename T>
struct unwrap_impl_default <T, Require<Nullable<T>>> : unwrap_with_dereference {};

} // namespace detail


template <typename T>
struct unwrap_impl : detail::unwrap_impl_default<T> {};

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
constexpr decltype(auto) unwrap(T&& v) noexcept(
        noexcept(unwrap_impl<std::decay_t<T>>::apply(std::forward<T>(v)))) {
    return unwrap_impl<std::decay_t<T>>::apply(std::forward<T>(v));
}

template <typename T>
struct unwrap_impl<std::reference_wrapper<T>> : detail::unwrap_with_get {};
template <>
struct unwrap_impl<std::nullptr_t> : detail::unwrap_with_forward {};
template <>
struct unwrap_impl<__OZO_NULLOPT_T> : detail::unwrap_with_forward {};
template <typename T>
struct unwrap_impl<std::weak_ptr<T>> : detail::unwrap_with_lock_dereference {};
template <typename T>
struct unwrap_impl<boost::weak_ptr<T>> : detail::unwrap_with_lock_dereference {};

template <typename T>
inline auto is_null(const T& v) noexcept -> Require<Nullable<T>, bool> {
    return !v;
}

template <typename T>
inline auto is_null(const std::weak_ptr<T>& v) noexcept(noexcept(is_null(v.lock()))) {
    return is_null(v.lock());
}

template <typename T>
inline auto is_null(const boost::weak_ptr<T>& v) noexcept(noexcept(is_null(v.lock()))) {
    return is_null(v.lock());
}

constexpr std::true_type is_null(std::nullptr_t) noexcept {return {};}

constexpr std::true_type is_null(__OZO_NULLOPT_T) noexcept {return {};}

#ifdef OZO_DOCUMENTATION
/**
 * @brief Indicates if value is in null state
 *
 * This utility function indicates if given value is in null state.
 * If argument is not #Nullable it always returns `false`
 *
 * @param value --- object to examine
 * @return `true` --- object is in null-state,
 * @return `false` --- overwise.
 * @ingroup group-core-functions
 */
template <typename T>
constexpr auto is_null(const T&) noexcept;
#endif

template <typename T>
constexpr auto is_null(const T&) noexcept -> Require<!Nullable<T>, std::false_type> {
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
    if (is_null(n)) {
        allocate_nullable(n, a);
    }
}

template <typename T>
inline void reset_nullable(T& n) {
    static_assert(Nullable<T>, "T must be nullable");
    n = T{};
}

/**
 * @brief Unwraps nullable or reference wrapped type
 * @ingroup group-type_system-types
 * Sometimes it is needed to know the underlying type of #Nullable or
 * type is wrapped with `std::reference_type`. So that is why the type exists.
 * Convenient shortcut is `ozo::unwrap_type`.
 * @tparam T -- type to unwrap.
 *
 * ### Example
 *
 * @code
int a = 42;
auto ref_a = std::ref(a);
auto opt_a = std::make_optional(a);

static_assert(std::is_same<ozo::unwrap_type<decltype(ref_a)>, decltype(a)>);
static_assert(std::is_same<ozo::unwrap_type<decltype(opt_a)>, decltype(a)>);
 * @endcode
 *
 * @sa ozo::unwrap_type, ozo::unwrap
 */
template <typename T>
struct get_unwrapped_type {
#ifdef OZO_DOCUMENTATION
    using type = <unwrapped type>; //!< Unwrapped type
#else
    using type = std::decay_t<decltype(unwrap(std::declval<T>()))>;
#endif
};

/**
 * @brief Shortcut for `ozo::unwrap_type`.
 * @ingroup group-type_system-types
 *
 * @tparam T -- type to unwrap.
 */
template <typename T>
using unwrap_type = typename get_unwrapped_type<T>::type;

/**
 * @brief Indicates if type is PostgreSQL array representation.
 * @ingroup group-type_system-types
 * Representation means that you can obtain PostgreSQL array into the supported
 * container.
 * Array by default can be represented via next containers:
 *   * `std::vector<T>`
 *   * `std::list<T>`
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
template <typename ...Ts>
struct is_array<std::vector<Ts...>> : std::true_type {};
template <typename ...Ts>
struct is_array<std::list<Ts...>> : std::true_type {};

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

template <typename ...Ts>
struct is_composite<std::tuple<Ts...>> : std::true_type {};

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

template <typename T>
constexpr auto DynamicSize = is_dynamic_size<std::decay_t<T>>::value;

template <typename T>
constexpr auto StaticSize = !DynamicSize<T>;

/**
* @brief Function returns type name in Postgre SQL.
* @tparam T --- type
* @return `boost::hana::string` --- name of the type
* @ingroup group-type_system-functions
*/
template <typename T>
constexpr auto type_name() noexcept {
    static_assert(!std::is_void<typename type_traits<T>::name>::value,
        "no type_traits found for the type");
    constexpr auto name = typename type_traits<T>::name{};
    constexpr char const* retval = hana::to<char const*>(name);
    return retval;
}

/**
* @brief Function returns type name in Postgre SQL.
* @param T --- type
* @return `boost::hana::string` --- name of the type
* @ingroup group-type_system-functions
*/
template <typename T>
constexpr auto type_name(const T&) noexcept {return type_name<T>();}

/**
 * @brief `ozo::size_of` implementation functor
 * @ingroup group-type_system-types
 *
 * This template is used to implement object binary representation size calculation
 * including all the meta-information is used for the PostgreSQL binary protocol.
 * By default it provides `static_assert` failure - which means that there is no
 * know algorithm to calculate size. There are default specializations for different
 * common types and concepts, incluiding `BuiltIn`, `Array`, `Composite`, `Nullable`.
 *
 * @tparam T --- type to examine.
 * @tparam <anonymous> --- SFINAE overloading rule.
 *
 * @note This template has to be defined in case of defining custom IO via `ozo::send_impl`
 * and `ozo::recv_impl`.
 *
 * ### Example
 *
 * This is how it can be implemented for the static size type (*exposition only*):
 *
 * @code
template <>
struct size_of_impl<std::nullptr_t> {
    static constexpr auto apply(std::nullptr_t) noexcept {
        return null_state_size;
    }
};
 * @endcode
 */
template <typename T, typename = std::void_t<>>
struct size_of_impl;

namespace detail {

template <typename T, typename = std::void_t<>>
struct size_of_default_impl {
    static constexpr auto apply(const T&) noexcept {
        return typename type_traits<T>::size{};
    }
};

template <typename T>
struct size_of_default_impl<T, Require<DynamicSize<T>>> {
    static constexpr auto apply(const T& v) noexcept(noexcept(std::size(v))){
        return std::size(v) * sizeof(decltype(*v.begin()));
    }
};

template <typename T, typename = std::void_t<>>
struct size_of_impl_dispatcher { using type = size_of_impl<std::decay_t<T>>; };

template <typename T>
using get_size_of_impl = typename size_of_impl_dispatcher<unwrap_type<T>>::type;

} // namespace detail
/**
 * @brief Returns size of object binary representation in bytes.
 * @ingroup group-type_system-functions
 *
 * This function returns binary representation size of the object
 * is used for the PostgreSQL binary protocol.
 *
 * @param v --- object to examine
 * @return `ozo::size_type` --- size of object in bytes if object is
 * not #Nullable or not in null state.
 * @return `ozo::null_state_size` --- for #Nullable object in null state.
 *
 * @note T has to have `ozo::type_traits` specialization.
 *
 * ### Customization point
 *
 * This function can be customized for the type or concept via `ozo::size_of_impl` structure
 * template specialization.
 *
 * @sa ozo::size_of_impl, OZO_PG_DEFINE_TYPE_AND_ARRAY, OZO_PG_DEFINE_CUSTOM_TYPE
 */
template <typename T>
constexpr size_type size_of(const T& v) {
    static_assert(HasDefinition<T>, "the type has not been defined with PostgreSQL type traits");
    return is_null(v) ? null_state_size : detail::get_size_of_impl<T>::apply(unwrap(v));
}

template <typename T, typename>
struct size_of_impl : detail::size_of_default_impl<T> {};
/**
 * @brief Namespace for PostgreSQL specific types
 */
namespace pg {

OZO_STRONG_TYPEDEF(std::string, name)
OZO_STRONG_TYPEDEF(std::vector<char>, bytea)

} // namespace pg


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

OZO_PG_DEFINE_TYPE(std::nullptr_t, "null", null_oid, decltype(null_state_size))
OZO_PG_DEFINE_TYPE(__OZO_NULLOPT_T, "null", null_oid, decltype(null_state_size))
/**
 * @brief Bool type mapping
 * @ingroup group-type_system-mapping
 */
OZO_PG_DEFINE_TYPE_AND_ARRAY(bool, "bool", BOOLOID, 1000, bytes<1>)
OZO_PG_DEFINE_TYPE_AND_ARRAY(char, "char", CHAROID, 1002, bytes<1>)
OZO_PG_DEFINE_TYPE_AND_ARRAY(ozo::pg::bytea, "bytea", BYTEAOID, 1001, dynamic_size)

OZO_PG_DEFINE_TYPE_AND_ARRAY(boost::uuids::uuid, "uuid", UUIDOID, 2951, bytes<16>)

OZO_PG_DEFINE_TYPE_AND_ARRAY(int64_t, "int8", INT8OID, 1016, bytes<8>)
OZO_PG_DEFINE_TYPE_AND_ARRAY(int32_t, "int4", INT4OID, INT4ARRAYOID, bytes<4>)
OZO_PG_DEFINE_TYPE_AND_ARRAY(int16_t, "int2", INT2OID, INT2ARRAYOID, bytes<2>)

OZO_PG_DEFINE_TYPE_AND_ARRAY(ozo::oid_t, "oid", OIDOID, OIDARRAYOID, bytes<4>)

OZO_PG_DEFINE_TYPE_AND_ARRAY(double, "float8", FLOAT8OID, 1022, bytes<8>)
OZO_PG_DEFINE_TYPE_AND_ARRAY(float, "float4", FLOAT4OID, FLOAT4ARRAYOID, bytes<4>)

OZO_PG_DEFINE_TYPE_AND_ARRAY(std::string, "text", TEXTOID, TEXTARRAYOID, dynamic_size)

OZO_PG_DEFINE_TYPE_AND_ARRAY(ozo::pg::name, "name", NAMEOID, 1003, dynamic_size)

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
