#pragma once

#include <tuple>
#include <vector>
#include <list>
#include <ozo/result.h>
/**
 * Useful shortcuts for typed unnamed results containers
 */
namespace ozo {

template <typename ... Ts>
using typed_row = std::tuple<Ts...>;

/**
 * @ingroup group-requests-types
 * @brief Shortcut for easy result container definition.
 *
 * This shortcut defines `std::vector` container for row tuples.
 * @note It is very important to keep a sequence of types according to fields in query statement (see the example below).
 *
 * ### Example
 *
@code{cpp}

// Query statement
const auto query =
    "SELECT        id    ,      name     FROM users_info WHERE amount>="_SQL + std::int64_t(25);
//                ----         ======
//                 V             V
ozo::rows_of<std::int64_t, std::string> rows;
//           ------------  ===========

ozo::request(ozo::make_connector(io, conn_info), query, ozo::into(rows), boost::asio::use_future);
@endcode
 * @tparam Ts --- types of columns in result
 */
template <typename ... Ts>
using rows_of = std::vector<typed_row<Ts...>>;

/**
 * @ingroup group-requests-types
 * @brief Shortcut for easy result container definition.
 *
 * This shortcut defines `std::list` container for row tuples.
 * @note It is very important to keep a sequence of types according to fields in query statement (see the example below).
 *
 * ### Example
 *
@code{cpp}

// Query statement
const auto query =
    "SELECT        id     ,      name     FROM users_info WHERE amount>="_SQL + std::int64_t(25);
//                ----          ======
//                 V              V
ozo::lrows_of<std::int64_t, std::string> rows;
//            ------------  ===========

ozo::request(ozo::make_connector(io, conn_info), query, ozo::into(rows), boost::asio::use_future);
@endcode
 * @tparam Ts --- types of columns in result
 */
template <typename ... Ts>
using lrows_of = std::list<typed_row<Ts...>>;

/**
 * @ingroup group-requests-functions
 * @brief Shortcut for create result container back inserter.
 *
 * This shortcut defines insert iterator for row container.
 *
 * ### Example
 *
@code{cpp}

// Query statement
const auto query = "SELECT id, name FROM users_info WHERE amount>="_SQL + std::int64_t(25);

ozo::rows_of<std::int64_t, std::string> rows;

ozo::request(ozo::make_connector(io, conn_info), query, ozo::into(rows), boost::asio::use_future);
@endcode
 * @param v --- container for rows
 */
template <typename T>
constexpr auto into(T& v) { return std::back_inserter(v);}

/**
 * @ingroup group-requests-functions
 * @brief Shortcut for create reference wrapper for `ozo::basic_result`.
 *
 * This shortcut creates insert iterator for row container.
 *
 * ### Example
 *
@code{cpp}

// Query statement
const auto query = "SELECT id, name FROM users_info WHERE amount>="_SQL + std::int64_t(25);

ozo::result res;

ozo::request(ozo::make_connector(io, conn_info), query, ozo::into(res), boost::asio::use_future);
@endcode
 * @param v --- `ozo::basic_result` object for rows.
 */
template <typename T>
constexpr auto into(basic_result<T>& v) { return std::ref(v);}

} // namespace ozo
