#pragma once

#include <boost/hana/optional.hpp>
#include <boost/hana/type.hpp>
#include <ozo/core/none.h>
#include <ozo/core/options.h>
#include <type_traits>

namespace ozo {

namespace hana = boost::hana;

/**
 * @ingroup group-connection-types
 * @brief 'type enum' for transaction isolation levels supported by PostgreSQL
 *
 * See the official documentation [on transaction isolation](https://www.postgresql.org/docs/11/transaction-iso.html) and
 * [transaction initiation](https://www.postgresql.org/docs/11/sql-set-transaction.html) for more information on guarantees every level makes
 * and how these options affect transaction behaviour.
 */
struct isolation_level {
    constexpr static hana::type<class serializable_tag> serializable{}; //!< SERIALIZABLE isolation level
    constexpr static hana::type<class repeatable_tag> repeatable_read{}; //!< REPEATABLE READ isolation level
    constexpr static hana::type<class read_committed_tag> read_committed{}; //!< READ COMMITTED isolation level
    constexpr static hana::type<class read_uncommitted> read_uncommitted{}; //!< READ UNCOMMITTED isolation level (treated like READ COMMITTED by PostgreSQL)
};

/**
 * @ingroup group-connection-types
 * @brief 'type enum' for transaction modes supported by PostgreSQL
 *
 * See the official documentation on [transaction initiation](https://www.postgresql.org/docs/11/sql-set-transaction.html)
 * for more information on how these options affect transaction behaviour.
 */
struct transaction_mode {
    constexpr static hana::type<class read_write_tag> read_write{}; //!< READ WRITE transaction mode
    constexpr static hana::type<class read_only_tag> read_only{}; //!< READ ONLY transaction mode
};

/**
 * @ingroup group-connection-types
 * @brief transaction deferrability indicator
 * @tparam V integral constant indicating the deferrability
 *
 * See the official documentation on [transaction initiation](https://www.postgresql.org/docs/11/sql-set-transaction.html)
 * for more information on how these options affect transaction behaviour.
 */
template <typename V>
struct deferrable_mode : V {
    using base = V;

    constexpr auto operator!() const noexcept {
        return deferrable_mode<typename std::negation<V>::type>{};
    }
};

constexpr deferrable_mode<std::true_type> deferrable;

/**
 * @ingroup group-connection-types
 * @brief options for transactions
 *
 * This options can be used with ozo::begin
 */
struct transaction_options {
    constexpr static option<class isolation_level_tag> isolation_level{}; //!< Transaction isolation level, see ozo::isolation_level
    constexpr static option<class mode_tag> mode{}; //!< Transaction mode, see ozo::transaction_mode
    constexpr static option<class deferrability_tag> deferrability{}; //!< Transaction deferrability, see ozo::deferrable_mode
};

} // ozo
