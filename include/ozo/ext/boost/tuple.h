#pragma once

#include <ozo/pg/definitions.h>

#include <boost/hana/ext/boost/tuple.hpp>
#include <boost/tuple/tuple.hpp>
/**
 * @defgroup group-ext-boost-tuple boost::tuple
 * @ingroup group-ext-boost
 * @brief [boost::tuple](https://www.boost.org/doc/libs/1_66_0/libs/tuple/doc/html/tuple_users_guide.html) support
 *
 *@code
#include <ozo/ext/boost/tuple.h>
 *@endcode
 *
 * `boost::tuple<T...>` is defined as a generic composite type. and mapped as PostgreSQL
 * `Record` type and its array.
 */
namespace ozo::definitions {

template <typename ...Ts>
struct type<boost::tuple<Ts...>> : pg::type_definition<decltype("record"_s)>{};

template <typename ...Ts>
struct array<boost::tuple<Ts...>> : pg::array_definition<decltype("record"_s)>{};

} // namespace ozo::definitions

namespace ozo {

template <typename ...Ts>
struct is_composite<boost::tuple<Ts...>> : std::true_type {};

} // namespace ozo
