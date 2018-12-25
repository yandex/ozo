#pragma once

#include <ozo/connection.h>
#include <libpq-fe.h>

namespace ozo {

template <typename T>
inline transaction_status get_transaction_status(T&& conn) {
    static_assert(Connection<T>, "T must be a Connection");

    if (is_null(conn)) {
        return transaction_status::unknown;
    }

    const auto v = PQtransactionStatus(get_native_handle(conn));
    switch (v) {
        case PQTRANS_UNKNOWN: return transaction_status::unknown;
        case PQTRANS_IDLE: return transaction_status::idle;
        case PQTRANS_ACTIVE: return transaction_status::active;
        case PQTRANS_INTRANS: return transaction_status::transaction;
        case PQTRANS_INERROR: return transaction_status::error;
    }
    throw std::invalid_argument("unsupported transaction state");
}

} // namespace ozo
