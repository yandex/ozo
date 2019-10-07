#pragma once

#include <ozo/core/options.h>
#include <ozo/detail/begin_statement_builder.h>
#include <ozo/transaction_status.h>

namespace ozo {

/**
 * @defgroup group-transaction Transaction
 * @brief Transaction related concepts, types and functions.
 */

/**
 * @defgroup group-transaction-types Types
 * @ingroup group-transaction
 * @brief Transaction related types
 */

/**
 * @defgroup group-transaction-functions Functions
 * @ingroup group-transaction
 * @brief Transaction related functions
 */

/**
 * @brief Transaction Connection model
 *
 * `Connection` concept model that describes the open transaction on the underlying
 * connection. The transaction is <u>not</u> an RAII object it does not control transaction
 * flow. It is dedicated to describe the current transaction and represent a transaction
 * on the type level.
 *
 * @warning Depending on the underlying `%Connection` the transaction object may model `Nullable`.
 *          If the object is in null state call of any member may lead to undefined behaviour.
 *
 * @tparam Connection --- the underlying `Connection` model type.
 * @tparam Options --- the transaction options type, see `ozo::transaction_options` for the details.
 *
 * @thread_safety{Depends on underlying `%Connection` model,Unsafe}
 * @ingroup group-transaction-types
 * @models{Connection}
 */
template <typename Connection, typename Options>
class transaction {
public:
    static_assert(ozo::Connection<Connection>, "Connection should model Connection concept");

    using handle_type = Connection; //! Underlying connection object type
    using lowest_layer_type = unwrap_type<handle_type>; //!< Lowest level `Connection` model type - fully unwrapped type
    using native_handle_type = typename lowest_layer_type::native_handle_type; //!< Native connection handle type
    using oid_map_type = typename lowest_layer_type::oid_map_type; //!< Oid map of types that are used with the connection
    using error_context_type = typename lowest_layer_type::error_context_type; //!< Additional error context which could provide context depended information for errors
    using executor_type = typename lowest_layer_type::executor_type; //!< The type of the executor associated with the object.
    using options_type = std::decay_t<Options>; //!< The type of options specified for the transaction

    /**
     * Construct a new uninitialized transaction object.
     *
     * If underlying `Connection` model is `Nullable` then `ozo::is_null()` should return
     * `true` for the constructed object.
     *
     * @warning The constructed object is in an uninitialized state, which means it shall not
     *          be used until being initialized with a valid transaction object.
     *          Calling any member of the uninitialized object leads to undefined behavior.
     */
    transaction() = default;

    /**
     * Construct a new transaction object.
     *
     * @note Constructing a transaction object does not open any transaction,
     *       use `ozo::begin()` instead to open a transaction with required options.
     *
     * @param connection --- connection object to perform transaction on.
     * @param options --- options of the transaction
     */
    transaction(Connection&& connection, Options options) noexcept(
        std::is_nothrow_move_constructible_v<Connection>
        && std::is_nothrow_move_constructible_v<Options>)
    : impl_(std::move(connection)), options_(std::move(options)) {}

    /**
     * Get native connection handle object.
     *
     * This function may be used to obtain the underlying representation of the connection.
     * This is intended to allow access to native `libpq` functionality that is not otherwise provided.
     *
     * @note The object should be initialized for this call.
     *
     * @return native_handle_type --- native connection handle.
     */
    native_handle_type native_handle() const noexcept {
        return lowest_layer().native_handle();
    }

    /**
     * Get a reference to an oid map object for types that are used with the connection.
     *
     * @note The object should be initialized for this call.
     *
     * @return const oid_map_type& --- reference on oid map object.
     */
    const oid_map_type& oid_map() const noexcept { return lowest_layer().oid_map();}

    template <typename Key, typename Value>
    void update_statistics(const Key& key, Value&& v) noexcept {
        lowest_layer().update_statistics(key, std::forward<Value>(v));
    }
    decltype(auto) statistics() const noexcept {
        return lowest_layer().statistics();
    }

    /**
     * Get the additional context object for an error that occurred during the last operation on the connection.
     *
     * @note The object should be initialized for this call.
     *
     * @return const error_context_type& --- additional context for the last error
     */
    const error_context_type& get_error_context() const noexcept {
        return lowest_layer().get_error_context();
    }
    /**
     * Set the additional error context object. This function may be used to provide additional context-depended
     * data that is related to the current operation error.
     *
     * @note The object should be initialized for this call.
     *
     * @param v --- new error context.
     */
    void set_error_context(error_context_type v = error_context_type{}) {
        lowest_layer().set_error_context(std::move(v));
    }

    /**
     * Get the executor associated with the object.
     *
     * @note The object should be initialized for this call.
     *
     * @return executor_type --- executor object.
     */
    executor_type get_executor() const noexcept {
        return lowest_layer().get_executor();
    }

    /**
     * Release ownership of the underlying `%Connection` model object.
     *
     * This function may be used to obtain the underlying `Connection` model.
     * After calling this function, the transaction object may not be used.
     * The caller is the owner for the connection.
     *
     * @warning In case of a transaction in progress, no commit or rollback will be made.
     *          The caller is fully responsible for further transaction management.
     *
     * @param x --- transaction to release connection from,
     *              the object should be initialized for this call.
     * @return underlying `Connection` model object
     */
    friend handle_type release_connection(transaction&& x) noexcept {
        return std::move(x.impl_);
    }

    /**
     * Asynchronously wait for the connection socket to become ready to write or to have pending error conditions.
     *
     * Typically this function is used by the library within the connection establishing process and operation execution.
     * Users should not use it directly other than for custom `libpq`-based opeartions.
     *
     * @note The object should be initialized for this call.
     *
     * @param handler --- wait handler with `void(ozo::error_code, int=0)` signature.
     */
    template <typename WaitHandler>
    void async_wait_write(WaitHandler&& handler) {
        lowest_layer().async_wait_write(std::forward<WaitHandler>(handler));
    }

    /**
     * Asynchronously wait for the connection socket to become ready to read or to have pending error conditions.
     *
     * Typically this function is used by the library within the connection establishing process and operation execution.
     * Users should not use it directly other than for custom `libpq`-based opeartions.
     *
     * @note The object should be initialized for this call.
     *
     * @param handler --- wait handler with `void(ozo::error_code, int=0)` signature.
     */
    template <typename WaitHandler>
    void async_wait_read(WaitHandler&& handler) {
        lowest_layer().async_wait_read(std::forward<WaitHandler>(handler));
    }

    /**
     * Close the connection.
     *
     * Any asynchronous operations will be cancelled immediately,
     * and will complete with the `boost::asio::error::operation_aborted` error.
     *
     * @note The object should be initialized for this call.
     *
     * @return error_code - indicates what error occurred, if any. Note that,
     *                      even if the function indicates an error, the underlying
     *                      connection is closed.
     */
    error_code close() noexcept { return lowest_layer().close();}

    /**
     * Cancel all asynchronous operations associated with the connection.
     *
     * This function causes all outstanding asynchronous operations to finish immediately,
     * and the handlers for cancelled operations will be passed the `boost::asio::error::operation_aborted` error.
     *
     * @note The object should be initialized for this call.
     */
    void cancel() noexcept { lowest_layer().cancel();}

    /**
     * Determine whether the connection is in bad state.
     *
     * @note The object should be initialized for this call.
     *
     * @return false --- connection established, and it is ok to execute operations
     * @return true  --- connection is not established, no operation shall be performed,
     *                   but an error context may be obtained via `get_error_context()`
     *                   and `ozo::error_message()`.
     */
    bool is_bad() const noexcept { return lowest_layer().is_bad();}

    /**
     * Determine whether the connection may be used for operations.
     * The object may be used in noninitialized state for this call.
     *
     * @return true  --- connection established, and it is ok to execute operations
     * @return false --- connection is not established, no operation shall be performed,
     *                   but an error context may be obtained via `get_error_context()`
     *                   and `ozo::error_message()`.
     */
    operator bool () const noexcept { return !(is_null() || is_bad());}

    /**
     * Determine whether the connection is open.
     * The object may be used in noninitialized state for this call.
     *
     * @return false --- connection is closed and no native handle associated with.
     * @return true  --- connection is open and there is a native handle associated with.
     */
    bool is_open() const noexcept { return !is_null() && lowest_layer().is_open();}

    /**
     * Get the transaction options. See `ozo::transaction_options` for the details.
     *
     * @note The object should be initialized for this call.
     */
    constexpr const options_type& options() const noexcept { return options_;}

    /**
     * Get a reference to the lowest layer.
     *
     * @note The object should be initialized for this call.
     */
    lowest_layer_type& lowest_layer() noexcept { return ozo::unwrap(impl_);}

    /**
     * Get a const reference to the lowest layer.
     *
     * @note The object should be initialized for this call.
     */
    const lowest_layer_type& lowest_layer() const noexcept {return ozo::unwrap(impl_);}

private:
    bool is_null() const noexcept { return ozo::is_null(impl_); }

    friend struct is_null_impl<transaction>;
    handle_type impl_;
    options_type options_;
};

template <typename ...Ts>
struct is_connection<transaction<Ts...>> : std::true_type {};

namespace impl {
// [[DEPRECATED]] for backward compatibility only
template <typename ...Ts>
using transaction = ozo::transaction<Ts...>;
} // namespace impl

namespace detail {
struct initiate_async_start_transaction {
    template <typename Handler, typename ...Args>
    constexpr void operator()(Handler&& h, Args&& ...args) const;
};
} // namespace detail

#ifdef OZO_DOCUMENTATION
/**
 * @brief Start new transaction
 *
 * The function starts new transaction on a database. The function can
 * be called as any of Boost.Asio asynchronous function with #CompletionToken. The operation would be
 * cancelled if time constrain is reached while performing.
 *
 * @note The function does not particitate in ADL since could be implemented via functional object.
 *
 * @param provider --- `ConnectionProvider` to get connection from.
 * @param time_constraint --- operation `TimeConstraint`; this time constrain <b>includes</b> time for getting connection from provider.
 * @param token --- operation #CompletionToken.
 * @return deduced from #CompletionToken.
 *
 * @par Transaction options
 *
 * Transaction may be started with specialized options like isolation level, mode and so on.
 * To specify options additional call of `with_transaction_options` should be used.
 *
 * @code
ozo::begin.with_transaction_options(ozo::make_options(Options...));
 * @endcode
 *
 * there `%Options` are available items of `ozo::transaction_options`.
 *
 * @par Example
 *
 * For full example see [examples/transaction.cpp](examples/transaction.cpp).
 *
 * Beginning a transaction:
@snippet examples/transaction.cpp Beginning a transaction
 * @ingroup group-transaction-functions
 */
template <typename ConnectionProvider, typename TimeConstraint, typename CompletionToken>
decltype(auto) begin (ConnectionProvider&& provider, TimeConstraint time_constraint, CompletionToken&& token);

/**
 * @brief Start new transaction
 *
 * This function is time constrain free shortcut to `ozo::begin()` function.
 * Its call is equal to `ozo::begin(provider, ozo::none, out, token)` call.
 *
 * @note The function does not particitate in ADL since could be implemented via functional object.
 *
 * @param provider --- #ConnectionProvider to get connection from.
 * @param token --- operation #CompletionToken.
 * @return deduced from #CompletionToken.
 * @ingroup group-transaction-functions
 */
template <typename ConnectionProvider, typename CompletionToken>
decltype(auto) begin (ConnectionProvider&& provider, CompletionToken&& token);

#endif
//! @cond
template <typename Initiator, typename Options = decltype(make_options())>
struct begin_op : base_async_operation <begin_op<Initiator, Options>, Initiator> {
    using base = typename begin_op::base;
    Options options_;

    constexpr explicit begin_op(Initiator initiator = {}, Options options = {}) : base(initiator), options_(options) {}

    template <typename T, typename TimeConstraint, typename CompletionToken>
    auto operator() (T&& provider, TimeConstraint t, CompletionToken&& token) const {
        static_assert(ConnectionProvider<T>, "provider should be a ConnectionProvider");
        static_assert(ozo::TimeConstraint<TimeConstraint>, "should model TimeConstraint concept");
        using namespace ozo::literals;
        return async_initiate<CompletionToken, handler_signature<transaction<connection_type<T>, Options>>>(
            get_operation_initiator(*this), token,
            std::forward<T>(provider), options_, detail::begin_statement_builder::build(options_), t);
    }

    template <typename T, typename CompletionToken>
    auto operator() (T&& provider, CompletionToken&& token) const {
        return (*this)(
            std::forward<T>(provider),
            none,
            std::forward<CompletionToken>(token)
        );
    }

    template <typename OtherOptions>
    constexpr auto with_transaction_options(const OtherOptions& options) const {
        return begin_op<Initiator, OtherOptions>{get_operation_initiator(*this), options};
    }

    template <typename OtherInitiator>
    constexpr auto rebind_initiator(const OtherInitiator& other) const {
        return begin_op<OtherInitiator, Options>{other, options_};
    }
};

inline constexpr begin_op<detail::initiate_async_start_transaction> begin;

//! @endcond

namespace detail {
struct initiate_async_end_transaction {
    template <typename Handler, typename ...Args>
    constexpr void operator()(Handler&& h, Args&& ...args) const;
};
} // namespace detail

#ifdef OZO_DOCUMENTATION
/**
 * @brief Commits a transaction
 *
 * The function commits transaction on a database. The function can
 * be called as any of Boost.Asio asynchronous function with #CompletionToken.
 * The operation would be cancelled if time constrain is reached while performing.
 *
 * @note The function does not particitate in ADL since could be implemented via functional object.
 *
 * @note After commit the transaction object may not be used.
 *
 * @param transaction --- open transaction to commit.
 * @param time_constraint --- operation `TimeConstraint`.
 * @param token --- operation `CompletionToken`.
 * @return deduced from the `CompletionToken`.
 *
 * @par Example
 *
 * For full example see [examples/transaction.cpp](examples/transaction.cpp).
 *
 * Committing a transaction:
@snippet examples/transaction.cpp Committing a transaction
 * @ingroup group-transaction-functions
 */
template <typename T, typename Options, typename TimeConstraint, typename CompletionToken>
decltype(auto) commit (transaction<T, Options>&& transaction, TimeConstraint t, CompletionToken&& token);

/**
 * @brief Commits a transaction
 *
 * This function is time constrain free shortcut to `ozo::commit()` function.
 * Its call is equal to `ozo::commit(std::move(transaction), ozo::none, token)` call.
 *
 * @note The function does not particitate in ADL since could be implemented via functional object.
 *
 * @param transaction --- open transaction to commit.
 * @param token --- operation `CompletionToken`.
 * @return deduced from the `CompletionToken`.
 * @ingroup group-transaction-functions
 */
template <typename ConnectionProvider, typename CompletionToken>
decltype(auto) commit (ConnectionProvider&& provider, CompletionToken&& token);

#endif
//! @cond
struct commit_op {
    template <typename T, typename Options, typename TimeConstraint, typename CompletionToken>
    auto operator() (transaction<T, Options>&& transaction, TimeConstraint t, CompletionToken&& token) const {
        static_assert(ozo::TimeConstraint<TimeConstraint>, "should model TimeConstraint concept");
        using namespace ozo::literals;
        return async_initiate<CompletionToken, handler_signature<typename ozo::transaction<T, Options>::handle_type>>(
            detail::initiate_async_end_transaction{}, token,
            std::move(transaction), "COMMIT"_SQL, t);
    }

    template <typename... Ts, typename CompletionToken>
    auto operator() (transaction<Ts...>&& transaction, CompletionToken&& token) const {
        return (*this)(
            std::move(transaction),
            none,
            std::forward<CompletionToken>(token)
        );
    }
};

inline constexpr commit_op commit;

//! @endcond

#ifdef OZO_DOCUMENTATION
/**
 * @brief Rollback a transaction
 *
 * The function rollback transaction on a database. The function can
 * be called as any of Boost.Asio asynchronous function with #CompletionToken.
 * The operation would be cancelled if time constrain is reached while performing.
 *
 * @note The function does not particitate in ADL since could be implemented via functional object.
 *
 * @note After rollback the transaction object may not be used.
 *
 * @param transaction --- open transaction to rollback.
 * @param time_constraint --- operation `TimeConstraint`.
 * @param token --- operation `CompletionToken`.
 * @return deduced from the `CompletionToken`.
 *
 * @par Example
 *
 * For full example see [examples/transaction.cpp](examples/transaction.cpp).
 *
 * Committing a transaction:
@code
conn = ozo::rollback(std::move(transaction), yield[ec]);
@endcode
 * @ingroup group-transaction-functions
 */
template <typename T, typename Options, typename TimeConstraint, typename CompletionToken>
decltype(auto) rollback (transaction<T, Options>&& transaction, TimeConstraint t, CompletionToken&& token);

/**
 * @brief Commits a transaction
 *
 * This function is time constrain free shortcut to `ozo::rollback()` function.
 * Its call is equal to `ozo::rollback(std::move(transaction), ozo::none, token)` call.
 *
 * @note The function does not particitate in ADL since could be implemented via functional object.
 *
 * @param transaction --- open transaction to rollback.
 * @param token --- operation `CompletionToken`.
 * @return deduced from the `CompletionToken`.
 * @ingroup group-transaction-functions
 */
template <typename ConnectionProvider, typename CompletionToken>
decltype(auto) rollback (ConnectionProvider&& provider, CompletionToken&& token);

#endif
//! @cond
struct rollback_op {
    template <typename T, typename Options, typename TimeConstraint, typename CompletionToken>
    auto operator() (transaction<T, Options>&& transaction, TimeConstraint t, CompletionToken&& token) const {
        static_assert(ozo::TimeConstraint<TimeConstraint>, "should model TimeConstraint concept");
        using namespace ozo::literals;
        return async_initiate<CompletionToken, handler_signature<typename ozo::transaction<T, Options>::handle_type>>(
            detail::initiate_async_end_transaction{}, token,
            std::move(transaction), "ROLLBACK"_SQL, t);
    }

    template <typename... Ts, typename CompletionToken>
    auto operator() (transaction<Ts...>&& transaction, CompletionToken&& token) const {
        return (*this)(
            std::move(transaction),
            none,
            std::forward<CompletionToken>(token)
        );
    }
};

inline constexpr rollback_op rollback;

//! @endcond

/**
 * @ingroup group-transaction-functions
 * @brief Retrieve's a transactions isolation level
 *
 * The isolation level can either be an instance of one of the child types of ozo::isolation_level or
 * `ozo::none` if no level was explicitly set.
 *
 * @param transaction transaction to get the isolation level from
 * @return sub type of ozo::isolation_level --- if the transaction isolation level has been set
 * @return ozo::none --- if no isolation level has been set
 */
template<typename... Ts>
constexpr auto get_transaction_isolation_level(const transaction<Ts...>& transaction) {
    return get_option(transaction.options(), transaction_options::isolation_level, none);
}

/**
 * @ingroup group-transaction-functions
 * @brief Retrieve's a transactions transaction mode
 *
 * The mode can either be an instance of one of the child types of ozo::transaction_mode or
 * ozo::none if mode was set explicitly set.
 *
 * @param transaction transaction to get the transaction mode from
 * @return sub type of ozo::transaction_mode --- if the transaction mode has been set
 * @return ozo::none --- if no transaction mode has been specified
 */
template<typename... Ts>
constexpr auto get_transaction_mode(const transaction<Ts...>& transaction) {
    return get_option(transaction.options(), transaction_options::mode, none);
}

/**
 * @ingroup group-transaction-functions
 * @brief Retrieve's a transactions deferrability
 *
 * Deferrability is usually indicated using an instance of ozo::deferrable_mode, but can also
 * be any other type with a compile-time static ::value convertible to bool.
 * `ozo::none` indicates no explicit deferrability was set.
 *
 * @param transaction transaction to get the deferrability from
 * @return ozo::deferrable_mode (or any bool-convertible std::integral_constant) --- if the deferrability has been set
 * @return ozo::none --- if no deferrability has been set
 */
template<typename... Ts>
constexpr auto get_transaction_deferrability(const transaction<Ts...>& transaction) {
    return get_option(transaction.options(), transaction_options::deferrability, none);
}

} // namespace ozo

#include <ozo/impl/transaction.h>
