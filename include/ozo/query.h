#pragma once

#include <ozo/detail/functional.h>
#include <ozo/core/concept.h>
#include <ozo/type_traits.h>

/**
 * @defgroup group-query Queries
 * @brief Database queries related concepts, types and functions.
 */

/**
 * @defgroup group-query-concepts Concepts
 * @ingroup group-query
 * @brief Query-related concepts.
 */

/**
 * @defgroup group-query-types types
 * @ingroup group-query
 * @brief Query-related concepts.
 */

/**
 * @defgroup group-query-functions Functions
 * @ingroup group-query
 * @brief Query-related functions.
 */
namespace ozo {

template <typename T, typename = hana::when<true>>
struct to_const_char_impl {};

template <typename T>
struct to_const_char_impl<T, hana::when<HanaString<T>>> {
    static constexpr const char* apply(const T& v) noexcept {
        return hana::to<const char*>(v);
    }
};

template <>
struct to_const_char_impl<std::string> {
    static const char* apply(const std::string& v) noexcept {
        return v.data();
    }
};

template <>
struct to_const_char_impl<std::string_view> {
    static constexpr const char* apply(std::string_view v) noexcept {
        return v.data();
    }
};

template <>
struct to_const_char_impl<const char*> {
    static constexpr const char* apply(const char* v) noexcept {
        return v;
    }
};

#ifdef OZO_DOCUMENTATION
/**
 * @brief Convert QueryText to const char*
 *
 * Convert #QueryText to `const char*` for futher transfer to a database. This function
 * invokes `ozo::to_const_char_impl::apply()` and should be customized via
 * `ozo::to_const_char_impl` specialization. Built-in support for types:
 * * `const char*`,
 * * `std::string`,
 * * `std::string_view`,
 * * `boost::hana::string`.
 *
 * ### Customization point
 *
 * This point should be used to add support for your own query text type. E.g.:
 *
 * @code
namespace ozo {

template <>
struct to_const_char_impl<my_ns::my_string> {
    static decltype(auto) apply(const my_ns::my_string& text) noexcept {
        return text.get_data();
    }
};

} // namespace ozo
 * @endcode
 *
 * @note `ozo::to_const_char_impl::apply()` should be `noexcept`.
 *
 * @param text --- query text to transform to `const char*`
 * @return `const char*`
 * @ingroup group-query-functions
 */
template <typename T>
constexpr auto to_const_char(const T& text) noexcept;
#else
template <typename T>
inline constexpr detail::result_of<to_const_char_impl, T> to_const_char(const T& v) noexcept {
    static_assert(noexcept(detail::apply<to_const_char_impl>(v)),
        "to_const_char_impl::apply() should be noexcept");
    static_assert(std::is_same_v<const char*, detail::result_of<to_const_char_impl, T>>,
        "to_const_char_impl::apply() should return const char*");
    return detail::apply<to_const_char_impl>(v);
}
#endif

template <class, class = std::void_t<>>
struct is_query_text : std::false_type {};

template <class T>
struct is_query_text<T, std::void_t<
    decltype(ozo::to_const_char(std::declval<const T&>()))
>> : std::true_type {};

/**
 * @brief Query text concept
 *
 * The `QueryText` concept is very simple. Query text should be convertible into
 * a `const char*` representation. The conversion should not throw an exception.
 * It means that `query_text` models `QueryText` concept if this code is valid:
 *
 * @code
 const char* text = ozo::to_const_char(query_text);
 * @endcode
 *
 * To adapt a custom text representation as `QueryText` it is needed to customize
 * `ozo::to_const_char()` function via it's implementation. For more details see
 * the function documentation.
 * @hideinitializer
 * @sa ozo::to_const_char
 * @ingroup group-query-concepts
 */
template <typename T>
constexpr auto QueryText = is_query_text<std::decay_t<T>>::value;

template <typename T, typename = hana::when<true>>
struct get_query_text_impl;

#ifdef OZO_DOCUMENTATION
/**
 * @brief Get the query text object
 *
 * Provides read-only access to a query text with SQL statement. This function
 * invokes `ozo::get_query_text_impl::apply()` and should be customized via
 * `ozo::get_query_text_impl` specialization.
 *
 * ### Customization point
 *
 * This is one of two points (see also `ozo::get_query_params()`) which should be
 * used to support your own query type in the library. E.g.:
 *
 * @code
namespace ozo {

template <>
struct get_query_text_impl<my_ns::my_query> {
    static decltype(auto) apply(const my_ns::my_query& q) {
        return q.get_text();
    }
};

} // namespace ozo
 * @endcode
 *
 * @param query --- #Query object
 * @return #QueryText --- query text with single SQL statement
 * @sa ozo::get_query_params
 * @ingroup group-query-functions
 */
template <typename T>
constexpr auto get_query_text(T&& query);
#else
template <typename T>
inline constexpr detail::result_of<get_query_text_impl, T> get_query_text(T&& query) {
    return detail::apply<get_query_text_impl>(std::forward<T>(query));
}
#endif

template <typename T, typename = hana::when<true>>
struct get_query_params_impl;

#ifdef OZO_DOCUMENTATION
/**
 * @brief Get the query parameters
 *
 * Provides read-only access to query parameters. This function invokes
 * `ozo::get_query_params_impl::apply()` and should be customized via
 * `ozo::get_query_params_impl` specialization.
 *
 * ### Customization point
 *
 * This is one of two points (see also `ozo::get_query_text()`) which should be
 * used to support your own query type in the library. E.g.:
 *
 * @code
namespace ozo {

template <>
struct get_query_params_impl<my_ns::my_query> {
    static decltype(auto) apply(const my_ns::my_query& q) {
        return boost::hana::make_tuple(q.param1, q.param2, q.param3);
    }
};

} // namespace ozo
 * @endcode
 * @param query --- #Query object
 * @return #HanaSequence --- tuple with query parameters for its' #QueryText
 * @sa ozo::get_query_text
 * @ingroup group-query-functions
 */
template <typename T>
constexpr auto get_query_params(T&& query);
#else
template <typename T>
inline constexpr detail::result_of<get_query_params_impl, T> get_query_params(T&& query) {
    return detail::apply<get_query_params_impl>(std::forward<T>(query));
}
#endif

template <class, class = std::void_t<>>
struct is_query : std::false_type {};

template <class T>
struct is_query<T, std::void_t<
    decltype(get_query_text(std::declval<const T&>())),
    decltype(get_query_params(std::declval<const T&>()))
>> : std::bool_constant<
    QueryText<decltype(get_query_text(std::declval<const T&>()))>
    && HanaSequence<decltype(get_query_params(std::declval<const T&>()))>
> {};

/**
 * @brief Query concept
 *
 * Query consists of two parts:
 * * text, which should models #QueryText concept,
 * * parameters sequence, whish should models #HanaSequence.
 *
 * This is very simple but powerful concept, which allows to use different
 * query representations with the library. Type `query` models `Query` concept
 * if this code is valid:
 *
 * @code
static_assert(ozo::QueryText<decltype(ozo::get_query_text(query))>);
static_assert(ozo::HanaSequence<decltype(ozo::get_query_params(query))>);
 * @endcode
 *
 * To adapt custom type as #Query for the library it is needed to customize
 * `ozo::get_query_text()` and `ozo::get_query_params()` via its' implementations.
 * See both functions description for more details.
 *
 * Query may be constructed via `ozo::make_query` function or `ozo::query_builder`.
 *
 * @hideinitializer
 * @ingroup group-query-concepts
 */
template <typename T>
constexpr auto Query = is_query<std::decay_t<T>>::value;

/**
 * @brief Construct built-in Query implementation object
 *
 * Construct built-in #Query implementation object from specified text and
 * variadic parameters.
 *
 * @note Currently no compile-time parameters validation provided by
 * this function, the validation would be made in run-time only.
 *
 * ### Example
 *
 * @code
auto query = ozo::make_query(
    "SELECT id, name FROM users WHERE credit > $1 AND rating > $2;",
    min_credit, min_rating);
 * @endcode
 *
 * @param text --- #QueryText implementation
 * @param params --- variadic parameters of the query
 * @return #Query object
 * @ingroup group-query-functions
 */
template <class Text, class ... ParamsT>
inline constexpr auto make_query(Text&& text, ParamsT&& ... params);

template <class ... ParamsT>
inline constexpr auto make_query(const char* text, ParamsT&& ... params) {
    return make_query(std::string_view(text), std::forward<ParamsT>(params)...);
}

template <class T, class = Require<Query<T>>>
decltype(auto) get_text(const T& query) {
    return get_query_text(query);
}

template <class T, class = Require<Query<T>>>
decltype(auto) get_params(const T& query) {
    return get_query_params(query);
}

} // namespace ozo

#include <ozo/impl/query.h>
