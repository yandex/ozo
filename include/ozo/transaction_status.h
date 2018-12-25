#pragma once

#include <ozo/connection.h>

namespace ozo {

/**
 * @brief Transaction status of a #Connection
 *
 * Indicates current transaction status of a #Connection object. It reflects (but not equal!) libpq
 * <a href="https://www.postgresql.org/docs/current/libpq-status.html">PGTransactionStatusType</a>.
 *
 * @note In case of developing own connection pool, it must be taking in account what #Connection
 * can be reused only with `ozo::transaction_status::idle` status, in case of the other status
 * the #Connection shall be closed rather than collected back into the pool.
 *
 * @ingroup group-connection-types
 */
enum class transaction_status {
    unknown,  //!< transaction state is unknown due to bad or invalid connection object, reflects `PQTRANS_UNKNOWN`
    idle, //!< connection is in idle state and can be reused, reflects `QTRANS_IDLE`
    active, //!< command execution is in progress, reflects `PQTRANS_ACTIVE`
    transaction, //!< idle, but within transaction block, reflects `PQTRANS_INTRANS`
    error, //!< idle, within failed transaction, reflects `PQTRANS_INERROR`
};

/**
 * @brief Returns current status of a #Connection object
 *
 * Returns current status of specified #Connection. If the #Connection is
 * #Nullable in null-state returns `ozo::transaction_status::unknown`. In other
 * case it returns value associated with libpq connection transaction status.
 *
 * @param conn --- #Connection
 * @return transaction_status --- status of the #Connection
 * @throws std::invalid_argument --- in case of unsupported value returned by libpq
 * function. It is better to use `-Werror` compiler flag to prevent such possability by
 * checking enumeration coverage in the `switch-case` statement.
 */
template <typename Connection>
inline transaction_status get_transaction_status(Connection&& conn);

} // namespace ozo

#include <ozo/impl/transaction_status.h>
