#pragma once

#ifndef BOOST_HANA_CONFIG_ENABLE_STRING_UDL
#error "OZO needs BOOST_HANA_CONFIG_ENABLE_STRING_UDL to be defined"
#endif

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
struct is_fusion_adapted_struct<T, std::enable_if_t<
    std::is_same_v<
        typename boost::fusion::traits::tag_of<T>::type,
        boost::fusion::struct_tag
    >
>> : std::true_type {};


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

template <typename T, std::size_t, typename = std::void_t<>>
struct has_data : std::false_type {};

template <typename T, std::size_t Size>
struct has_data<T, Size, std::void_t<decltype(std::declval<T&>().data())>> :
    std::bool_constant<sizeof(decltype(*std::declval<T&>().data())) == 1> {};

template <typename T, std::size_t, typename = std::void_t<>>
struct has_friend_data : std::false_type {};

template <typename T, std::size_t Size>
struct has_friend_data<T, Size, std::void_t<decltype(data(std::declval<T&>()))>> :
    std::bool_constant<sizeof(decltype(*data(std::declval<T&>()))) == 1> {};

template <typename T, typename = std::void_t<>>
struct has_size : std::false_type {};

template <typename T>
struct has_size<T, std::void_t<decltype(std::declval<T&>().size())>> : std::true_type {};

template <typename T, typename = std::void_t<>>
struct has_friend_size : std::false_type {};

template <typename T>
struct has_friend_size<T, std::void_t<decltype(size(std::declval<T&>()))>> : std::true_type {};

template <typename T>
struct is_raw_data_writable : std::bool_constant<
    (has_data<T, 1>::value && has_size<T>::value) ||
    (has_friend_data<T, 1>::value && has_friend_size<T>::value)
> {};


/**
 * @brief `RawDataWritable` concept
 *
 * Indicates if `T` can be written as a sequence of bytes without endian conversion.
 * `RawDataWritable<T>` is `true` if for object `v` of type `T` applicable one of this code:
 * @code
    auto raw = v.data();              // has_data<T,
    static_assert(sizeof(*raw) == 1); //             1>
    auto n = v.size();                // has_size<T>
 * @endcode
 * or
 * @code
    auto raw = data(v);               // has_friend_data<T,
    static_assert(sizeof(*raw) == 1); //                    1>
    auto n = size(v);                 // has_friend_size<T>
 * @endcode
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

/**
 * @brief Completion token concept
 *
 * `CompletionToken` is an entity which determines how to continue with asynchronous operation result when
 * the operation is complete. According to <a href="https://www.boost.org/doc/libs/1_66_0/doc/html/boost_asio/reference/async_completion.html">
 * `boost::asio::async_completion`</a> it defines the return value of an asynchronous function.
 *
 * Assume we have an asynchronous IO function:
 * @code
template <typename ConnectionProvider, typename CompletionToken>
auto async_io(ConnectionProvider&&, Param1 p1, ..., CompletionToken&&);
 * @endcode
 *
 * Then the result type of the function depends on `CompletionToken`, and `CompletionToken` - is any of these next entities:
 * * #Handler concept implementation. Asynchronous function in this case will return `void`.
 * In this case the equivalent function signature will be:
 * @code
template <typename ConnectionProvider>
void async_io(ConnectionProvider, Param1 p1, ..., Handler);
 * @endcode
 *
 * * <a href= "https://www.boost.org/doc/libs/1_66_0/doc/html/boost_asio/reference/use_future.html">
 * `boost::asio::use_future`</a> - to get a future on the asynchronous operation result.
 * Asynchronous function in this case will return `std::future<Connection>`.
 * In this case the equivalent function signature will be:
 * @code
template <typename ConnectionProvider>
std::future<ozo::connection_type<ConnectionProvider>> async_io(
    ConnectionProvider&&, Param1 p1, ..., boost::asio::use_future_t);
 * @endcode
 *
 * * <a href="https://www.boost.org/doc/libs/1_66_0/doc/html/boost_asio/reference/basic_yield_context.html">
 * `boost::asio::yield_context`</a> - to use async operation with Boost.Coroutine.
 * Asynchronous function in this case will return #Connection.
 * In this case the equivalent function signature will be:
 * @code
template <typename ConnectionProvider>
ozo::connection_type<ConnectionProvider> async_io(
    ConnectionProvider&&, Param1 p1, ..., boost::asio::yield_context);
 * @endcode
 *
 * * any other type supported with <a href="https://www.boost.org/doc/libs/1_66_0/doc/html/boost_asio/reference/async_completion.html">
 * `boost::asio::async_completion`</a> mechanism.
 * Asynchronous function in this case will return a type is depends on
 * <a href="https://www.boost.org/doc/libs/1_66_0/doc/html/boost_asio/reference/async_completion/result.html">
 * `boost::asio::async_completion::result`</a>.
 * @hideinitializer
 */
#ifdef OZO_DOCUMENTATION
template <typename T>
constexpr auto CompletionToken = std::false_type;
#endif

/**
 * @brief Handler concept
 *
 * `Handler` is a function or a functor which is used as a callback for handling result of asynchronous IO operations in the library.
 *
 * In case of function it has to have this signature:
 *@code
template <typename Connection>
void Handler(ozo::error_code ec, Connection connection) {
    //...
}
 *@endcode
 *
 * In case of functor it has to have such `operator()`:
 *@code
struct Handler {
    template <typename Connection>
    void operator() (ozo::error_code ec, Connection connection) {
        //...
    }
};
 *@endcode
 *
 * In case of lambda:
 *@code
auto Handler = [&] (ozo::error_code ec, auto connection) {
    //...
};
 *@endcode
 *
 * `Handler` has to handle an `ozo::error_code` object as first argument, and #Connection implementation
 * object as a second one. It is better to define second argument as a template parameter because the
 * implementation depends on a numerous of compile-time options but if it is really needed - real type
 * can be obtained with `ozo::connection_type`.
 *
 * `Handler` has to be invoked according to this rules:
 * * **Operation succeeded** --- `ozo::error_code` is empty, #Connection is in good state and can be used for next IO.
 * * **Operation failed** --- `ozo::error_code` contains error, #Connection can be in these states:
 *   * #Connection in null-state --- `ozo::is_null()` returns `true`, object is useless;
 *   * #Connection in bad state --- `ozo::is_null()` returns `false`, object may provide additional error context via
 *                   `ozo::error_message()` and `ozo::get_error_context()` functions.
 * @hideinitializer
 */
#ifdef OZO_DOCUMENTATION
template <typename T>
constexpr auto Handler = std::false_type;
#endif
///@}

} // namespace ozo
