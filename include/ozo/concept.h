#pragma once

#include <boost/fusion/adapted.hpp>
#include <boost/fusion/sequence.hpp>
#include <boost/fusion/support/is_sequence.hpp>
#include <boost/fusion/include/is_sequence.hpp>
#include <boost/hana/core/is_a.hpp>
#include <boost/hana/tuple.hpp>
#include <boost/hana/string.hpp>
#include <typeinfo>
#include <type_traits>
#include <iterator>

namespace ozo {

/**
 * @defgroup group-core-concepts Concepts
 * @ingroup group-core
 * @brief Library-wide concepts emulation mechanism
 *
 * We decide to use Concept-style meta programming to make easy to extend, adapt and test the
 * library. So here is our own reinvented wheel of C++ Concepts built on template constants and
 * std::enable_if.
 */
///@{
/**
* @brief Concept requirement emulation
*
* This is requirement simulation type, which is the alias to std::enable_if_t.
* It is pretty simple to use it with pseudo-concepts such as OperatorNot, Iterable and so on.
* E.g. overloading functions via Require:
* @code
    template <typename T>
    decltype(auto) unwrap(T&& c, Require<!Nullable<T>>* = 0) {
        return c;
    }

    template <typename T>
    decltype(auto) unwrap(T&& c, Require<Nullable<T>>* = 0) {
        return *c;
    }
* @endcode
*
* @tparam Condition - logical combination of concepts.
* @tparam Type - return type, *void* by default.
* @returns Type if Condition is true
*/
template <bool Condition, typename Type = void>
#ifdef OZO_DOCUMENTATION
using Require = Type;
#else
using Require = std::enable_if_t<Condition, Type>;
#endif


template <typename T, typename = std::void_t<>>
struct has_operator_not : std::false_type {};
template <typename T>
struct has_operator_not<T, std::void_t<decltype(!std::declval<T>())>>
    : std::true_type {};

/**
 * @brief Operator Not concept
 *
 * Return true if T has operator !(), false in other case
 * @tparam T - type to examine
 * @hideinitializer
 */
template <typename T>
constexpr auto OperatorNot = has_operator_not<std::decay_t<T>>::value;

template <typename T, typename Enable = void>
struct is_output_iterator : std::false_type {};

template <typename T>
struct is_output_iterator<T, typename std::enable_if<
    std::is_base_of<
        std::output_iterator_tag,
        typename std::iterator_traits<T>::iterator_category
    >::value
>::type>
: std::true_type {};

/**
 * @brief Output Iterator concept
 *
 * Returns true if T is output iterator
 * @tparam T - type to examine
 * @hideinitializer
 */
template <typename T>
constexpr auto OutputIterator = is_output_iterator<T>::value;

template <typename T, typename Enable = void>
struct is_forward_iterator : std::false_type {};

template <typename T>
struct is_forward_iterator<T, typename std::enable_if<
    std::is_base_of<
        std::forward_iterator_tag,
        typename std::iterator_traits<T>::iterator_category
    >::value
>::type>
: std::true_type {};

/**
 * @brief Forward Iterator concept
 *
 * Returns true if T meets forward_iterator conditions
 * @tparam T - type to examine
 * @hideinitializer
 */
template <typename T>
constexpr auto ForwardIterator = is_forward_iterator<T>::value;

template <typename T, typename Enable = void>
struct is_iterable : std::false_type {};

template <typename T>
struct is_iterable<T, typename std::enable_if<
    is_forward_iterator<decltype(begin(std::declval<T>()))>::value &&
        is_forward_iterator<decltype(end(std::declval<T>()))>::value
>::type>
: std::true_type {};

/**
 * @brief Iterable concept
 *
 * Determines whether T can be iterated through via begin() end() functions
 * @tparam T - type to examine
 * @hideinitializer
 */
template <typename T>
constexpr auto Iterable = is_iterable<T>::value;

template <typename T, typename Enable = void>
struct is_insert_iterator : std::false_type {};

template <typename T>
struct is_insert_iterator<T, typename std::enable_if<
    is_output_iterator<T>::value && std::is_class<typename T::container_type>::value
>::type>
: std::true_type {};

/**
 * @brief Insert Iterator concept
 *
 * This trait determines whether T is an insert iterator bound with some container
 * @tparam T - type to examine
 * @hideinitializer
 */
template <typename T>
constexpr auto InsertIterator = is_insert_iterator<T>::value;

/**
 * @brief Boost.Fusion Sequence concept
 *
 * Indicates if T is a Boost.Fusion Sequence type
 * @tparam T - type to examine
 * @hideinitializer
 */
template <typename T>
constexpr auto FusionSequence = boost::fusion::traits::is_sequence<std::decay_t<T>>::value;

/**
 * @brief Boost.Hana Sequence concept
 *
 * Indicates if T is a Boost.Hana Sequence type
 * @tparam T - type to examine
 * @hideinitializer
 */
template <typename T>
constexpr auto HanaSequence = boost::hana::Sequence<std::decay_t<T>>::value;

/**
 * @brief Boost.Hana Structure concept
 *
 * Indicates if T is a Boost.Hana Structure type
 * @tparam T - type to examine
 * @hideinitializer
 */
template <typename T>
constexpr auto HanaStruct = boost::hana::Struct<std::decay_t<T>>::value;

/**
 * @brief Boost.Hana String concept
 *
 * Indicates if T is a Boost.Hana String type
 * @tparam T - type to examine
 * @hideinitializer
 */
template <typename T>
constexpr auto HanaString = decltype(boost::hana::is_a<boost::hana::string_tag>(std::declval<T>()))::value;

/**
 * @brief Boost.Hana Tuple concept
 *
 * Indicates if T is a Boost.Hana Tuple type
 * @tparam T - type to examine
 * @hideinitializer
 */
template <typename T>
constexpr auto HanaTuple = decltype(boost::hana::is_a<boost::hana::tuple_tag>(std::declval<T>()))::value;

template <typename T, typename = std::void_t<>>
struct is_fusion_adapted_struct : std::false_type {};

template <typename T>
struct is_fusion_adapted_struct<T,
    std::void_t<decltype(boost::fusion::extension::struct_member_name<T, 0>::call())>
> : std::true_type {};


/**
 * @brief Boost.Fusion Adapted Structure concept
 *
 * Indicates it T is a Boost.Fusion adapted structure via BOOST_FUSION_ADAPT_STRUCT or similar.
 * @tparam T - type to examine
 * @hideinitializer
 */
template <typename T>
constexpr auto FusionAdaptedStruct = is_fusion_adapted_struct<std::decay_t<T>>::value;

/**
 * @brief Integral concept
 *
 * Indicates if T is an integral type
 * @tparam T - type to examine
 * @hideinitializer
 */
template <typename T>
constexpr auto Integral = std::is_integral_v<std::decay_t<T>>;

/**
 * @brief Floating Point concept
 *
 * Indicates if T is a floating point type
 * @tparam T - type to examine
 * @hideinitializer
 */
template <typename T>
constexpr auto FloatingPoint = std::is_floating_point_v<std::decay_t<T>>;

template <typename T, typename = std::void_t<>>
struct is_raw_data_writable : std::false_type {};

template <typename T>
struct is_raw_data_writable<T, std::void_t<decltype(std::declval<T&>().data())>> : std::true_type {};

/**
 * @brief Raw Data Writable concept
 *
 * Indicates if T can be written as a sequence of bytes without endian conversion
 * @tparam T - type to examine
 * @hideinitializer
 */
template <typename T>
constexpr auto RawDataWritable = is_raw_data_writable<std::decay_t<T>>::value;

template <typename T, typename = std::void_t<>>
struct is_emplaceable : std::false_type {};

template <typename T>
struct is_emplaceable<T, std::void_t<decltype(std::declval<T&>().emplace())>> : std::true_type {};

/**
 * @brief Emplaceable concept
 *
 * Indicates if container T can emplace it's element with default constructor
 * @tparam T - type to examine
 * @hideinitializer
 */
template <typename T>
constexpr auto Emplaceable = is_emplaceable<std::decay_t<T>>::value;

///@}

} // namespace ozo
