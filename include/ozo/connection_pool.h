#pragma once

#include <ozo/connection_info.h>
#include <ozo/transaction_status.h>
#include <ozo/asio.h>
#include <ozo/connector.h>
#include <ozo/core/thread_safety.h>
#include <ozo/detail/connection_pool.h>

namespace ozo {

/**
 * @brief Connection pool configuration
 * @ingroup group-connection-types
 *
 * Configuration of the `ozo::connection_pool`, e.g. how many connection are in the pool,
 * how many queries can be in wait queue if all connections are used by another queries,
 * and how long to keep connection open.
 */
struct connection_pool_config {
    std::size_t capacity = 10; //!< maximum number of stored connections
    std::size_t queue_capacity = 128; //!< maximum number of queued requests to get available connection
    time_traits::duration idle_timeout = std::chrono::seconds(60); //!< time interval to close connection after last usage
    time_traits::duration lifespan = std::chrono::hours(24); //!< time interval to keep connection open
};

/**
 * @brief [[DEPRECATED]] Timeouts for the ozo::get_connection() operation
 * @ingroup group-connection-types
 *
 * Time restrictions to get connection from the `ozo::connection_pool`
 * @note This type is deprecated, use time constrained version of ozo::get_connection() or ozo::dedline for operations.
 */
struct /*[[deprecated]]*/ connection_pool_timeouts {
    time_traits::duration connect = std::chrono::seconds(10); //!< maximum time interval to establish to or wait for free connection with DBMS
    time_traits::duration queue = std::chrono::seconds(10); //!< [[IGNORED]]
};

/**
 * @brief Connection traits depend on a representation type
 *
 * @tparam Rep --- connection representation data type
 * @ingroup group-connection-types
 */
template <typename Rep>
struct connection_traits {
    using native_handle_type = typename Rep::native_handle_type; //!< Native connection handle type
    using oid_map_type = typename Rep::oid_map_type; //!< Oid map of types that are used with the connection
    using error_context_type = typename Rep::error_context_type; //!< Additional error context which could provide context depended information for errors
    using statistics_type = typename Rep::statistics_type; //!< Connection statistics to be collected
};

template <typename OidMap, typename Statistics = none_t>
class connection_rep {
public:
    using oid_map_type = OidMap;
    using native_handle_type = typename ozo::pg::conn::pointer;
    using statistics_type = Statistics;
    using error_context_type = std::string;

    const ozo::pg::conn& safe_native_handle() const & {return safe_handle_;}
    ozo::pg::conn& safe_native_handle() & {return safe_handle_;}

    const oid_map_type& oid_map() const & {return oid_map_;}
    oid_map_type& oid_map() & {return oid_map_;}

    const auto& statistics() const & {return statistics_;}

    template <typename Key, typename Value>
    void update_statistics(const Key&, Value&&) noexcept {
        static_assert(std::is_void_v<Key>, "update_statistics is not supperted");
    }

    const error_context_type& get_error_context() const noexcept {
        return error_context_;
    }
    void set_error_context(error_context_type v) {
        error_context_ = std::move(v);
    }

    connection_rep(
        ozo::pg::conn&& safe_handle,
        OidMap oid_map = OidMap{},
        error_context_type error_context = {},
        Statistics statistics = Statistics{})
    : safe_handle_(std::move(safe_handle)),
      oid_map_(std::move(oid_map)),
      error_context_(std::move(error_context)),
      statistics_(std::move(statistics)) {}
private:
    ozo::pg::conn safe_handle_;
    oid_map_type oid_map_;
    error_context_type error_context_;
    statistics_type statistics_;
};

/**
 * @brief Pool bound model for `Connection` concept
 *
 * `Connection` concept model which is bound to and may be obtained only from
 * the connection pool. On the `pooled_connection` object destruction, the
 * underlying handle that contains a connection will be returned to the handle-associated
 * connection pool. If the connection is in a bad state either its current transaction
 * status is different than `ozo::transaction_status::idle` then it will not return to
 * the pool and be closed. The class object is non-copyable.
 *
 * @tparam Rep      --- underlying connection pool representation for the real connection.
 * @tparam Executor --- the type of the executor is used to perform IO; currently only
 *                      `boost::asio::io_context::executor_type` is supported.
 *
 * @thread_safety{Safe,Unsafe}
 * @ingroup group-connection-types
 * @models{Connection}
 */
template <typename Rep, typename Executor = asio::io_context::executor_type>
class pooled_connection {
public:
    using rep_type = Rep; //!< Connection representation type
    using native_handle_type = typename connection_traits<rep_type>::native_handle_type; //!< Native connection handle type
    using oid_map_type = typename connection_traits<rep_type>::oid_map_type; //!< Oid map of types that are used with the connection
    using error_context_type = typename connection_traits<rep_type>::error_context_type; //!< Additional error context which could provide context depended information for errors
    using statistics_type = typename connection_traits<rep_type>::statistics_type; //!< Connection statistics to be collected
    using executor_type = Executor; //!< The type of the executor associated with the object.

    pooled_connection(const Executor& ex, Rep&& rep);

    /**
     * Get native connection handle object.
     *
     * This function may be used to obtain the underlying representation of the connection.
     * This is intended to allow access to native `libpq` functionality that is not otherwise provided.
     *
     * @return native_handle_type --- native connection handle.
     */
    native_handle_type native_handle() const noexcept;

    /**
     * Get a reference to an oid map object for types that are used with the connection.
     *
     * @return const oid_map_type& --- reference on oid map object.
     */
    const oid_map_type& oid_map() const noexcept { return ozo::unwrap(rep_).oid_map();}

    template <typename Key, typename Value>
    void update_statistics(const Key& key, Value&& v) noexcept {
        ozo::unwrap(rep_).update_statistics(key, std::forward<Value(v)>);
    }
    const statistics_type& statistics() const noexcept { return ozo::unwrap(rep_).statistics();}

    /**
     * Get the additional context object for an error that occurred during the last operation on the connection.
     *
     * @return const error_context& --- additional context for the error
     */
    const error_context_type& get_error_context() const noexcept {
        return ozo::unwrap(rep_).get_error_context();
    }

    /**
     * Set the additional error context object. This function may be used to provide additional context-depended
     * data that is related to the current operation error.
     *
     * @param v --- new error context.
     */
    void set_error_context(error_context_type v = {}) {
        ozo::unwrap(rep_).set_error_context(std::move(v));
    }

    /**
     * Get the executor associated with the object.
     *
     * @return executor_type --- executor object.
     */
    executor_type get_executor() const noexcept { return ex_; }

    /**
     * Asynchronously wait for the connection socket to become ready to write or to have pending error conditions.
     *
     * Typically this function is used by the library within the connection establishing process and operation execution.
     * Users should not use it directly other than for custom `libpq`-based opeartions.
     *
     * @warning The function is designated to the library operations use only. Do not call this function directly.
     *
     * @param handler --- wait handler with `void(ozo::error_code, int=0)` signature.
     */
    template <typename WaitHandler>
    void async_wait_write(WaitHandler&& handler);

    /**
     * Asynchronously wait for the connection socket to become ready to read or to have pending error conditions.
     *
     * Typically this function is used by the library within the connection establishing process and operation execution.
     * Users should not use it directly other than for custom `libpq`-based opeartions.
     *
     * @warning The function is designated to the library operations use only. Do not call this function directly.
     *
     * @param handler --- wait handler with `void(ozo::error_code, int=0)` signature.
     */
    template <typename WaitHandler>
    void async_wait_read(WaitHandler&& handler);

    /**
     * Close the connection.
     *
     * Any asynchronous operations will be cancelled immediately,
     * and will complete with the `boost::asio::error::operation_aborted` error.
     *
     * @return error_code - indicates what error occurred, if any. Note that,
     *                      even if the function indicates an error, the underlying
     *                      connection is closed.
     */
    error_code close() noexcept;

    /**
     * Cancel all asynchronous operations associated with the connection.
     *
     * This function causes all outstanding asynchronous operations to finish immediately,
     * and the handlers for cancelled operations will be passed the `boost::asio::error::operation_aborted` error.
     */
    void cancel() noexcept;

    /**
     * Determine whether the connection is in bad state.
     *
     * @return false --- connection established, and it is ok to execute operations
     * @return true  --- connection is not established, no operation shall be performed,
     *                   but an error context may be obtained via `get_error_context()`
     *                   and `ozo::error_message()`.
     */
    bool is_bad() const noexcept;

    /**
     * Determine whether the connection is not in bad state.
     *
     * @return true  --- connection established, and it is ok to execute operations
     * @return false --- connection is not established, no operation shall be performed,
     *                   but an error context may be obtained via `get_error_context()`
     *                   and `ozo::error_message()`.
     */
    operator bool () const noexcept { return !is_bad();}

    /**
     * Determine whether the connection is open.
     *
     * @return false --- connection is closed and no native handle associated with.
     * @return true  --- connection is open and there is a native handle associated with.
     */
    bool is_open() const noexcept { return native_handle() != nullptr;}

    ~pooled_connection();
private:
    using stream_type = typename detail::connection_stream<executor_type>::type;

    rep_type rep_;
    executor_type ex_;
    stream_type stream_;
};

template <typename ...Ts>
struct is_connection<pooled_connection<Ts...>> : std::true_type {};

template <typename T>
struct connection_traits<yamail::resource_pool::handle<T>> :
    connection_traits<typename yamail::resource_pool::handle<T>::value_type> {};

/**
 * @brief Connection pool implementation
 *
 * This is a simple implementation connection pool (<a href="https://en.wikipedia.org/wiki/Connection_pool">wikipedia</a>).
 * Connection pool allows to store established connections and reuse it to avoid a connect operation for each request.
 * It supports asynchronous request for connections using a queue with optional limits of capacity and wait time.
 * Also connection in the pool may be closed when it is not used for some time - idle timeout.
 *
 * This is how `connection_pool` handles user request to get a `Connection` object:
 *
 * * If there is a free connection --- it will be provided for a user immediately.
 * * If all connections are busy but its number less than the limit, `ConnectionSource` creates a new connection and provides it to a user.
 * * If all connections are busy and there is no room to create a new one --- the request will be placed into the internal queue to wait for the free connection.
 *
 * The request may be limited by time via optional `connection_pool_timeouts` argument of the `connection_pool::operator()`.
 *
 * `connection_pool` models `ConnectionSource` concept itself using underlying `ConnectionSource`.
 *
 * @tparam Source --- underlying `ConnectionSource` which is being used to create connection to a database.
 * @tparam ThreadSafety --- admissibility to use in multithreaded environment without additional synchronization.
 * Thread safe by default.
 *
 * ###Example
 *
 * See [examples/connection_pool.cpp](examples/connection_pool.cpp).
 *
 * Creating the connection_pool instance:
@snippet examples/connection_pool.cpp Creating Connection Pool
 *
 * Creating a ConnectionProvider from the connection_pool instance:
@snippet examples/connection_pool.cpp Creating Connection Provider
 *
 * @ingroup group-connection-types
 * @models{ConnectionSource}
 */
template <typename Source, typename ThreadSafety = std::decay_t<decltype(thread_safe)>>
class connection_pool {
    static_assert(ConnectionSource<Source>, "should model ConnectionSource concept");

public:
    using connection_rep_type = ozo::connection_rep<typename ozo::unwrap_type<ozo::connection_type<Source>>::oid_map_type>;

    using impl_type = detail::get_connection_pool_impl_t<connection_rep_type, ThreadSafety>;
    /**
     * Construct a new connection pool object
     *
     * @param source --- `ConnectionSource` object which is being used to create connection to a database.
     * @param config --- pool configuration.
     * @param thread_safety -- admissibility to use in multithreaded environment without additional synchronization.
     * Thread safe by default (`ozo::thread_safety<true>`).
     */
    connection_pool(Source source, const connection_pool_config& config, const ThreadSafety& /*thread_safety*/ = ThreadSafety{})
    : impl_(config.capacity, config.queue_capacity, config.idle_timeout, config.lifespan),
      source_(std::move(source)) {}

    /**
     * Type of connection depends on connection type of Source. The definition is used to model `ConnectionSource`
     */
    using connection_type = std::shared_ptr<pooled_connection<yamail::resource_pool::handle<connection_rep_type>>>;

    /**
     * Get connection is bound to the given `io_context` object.
     * This operation has a time constrain and would be interrupted if the time
     * constrain expired by cancelling IO on a `Connection` or wait operation in
     * the pool's queue.
     *
     * @param io --- `io_context` for the connection IO.
     * @param t --- operation time constraint.
     * @param handler --- #Handler.
     */
    template <typename TimeConstraint, typename Handler>
    void operator ()(io_context& io, TimeConstraint t, Handler&& handler);

    auto stats() const {
        return impl_.stats();
    }

    auto operator [](io_context& io) {
        return connection_provider(*this, io);
    }

private:

    auto queue_timeout(time_traits::time_point at) const {
        return time_left(at);
    }

    auto queue_timeout(time_traits::duration t) const {
        return t;
    }

    auto queue_timeout(none_t) const {
        return time_traits::duration(0);
    }

    impl_type impl_;
    Source source_;
};

//[[DEPRECATED]] for backward compatibility only
template <typename ...Ts>
[[deprecated]] auto make_connector(connection_pool<Ts...>& source, io_context& io, const connection_pool_timeouts& timeouts) {
    return bind_get_connection_timeout(source[io], timeouts.connect);
}

//[[DEPRECATED]] for backward compatibility only
template <typename ...Ts>
[[deprecated]] auto make_connector(connection_pool<Ts...>& source, io_context& io) {
    return source[io];
}

static_assert(ConnectionProvider<decltype(std::declval<connection_pool<connection_info<>>>()[std::declval<io_context&>()])>, "is not a ConnectionProvider");

template <typename T>
struct is_connection_pool : std::false_type {};

template <typename ...Args>
struct is_connection_pool<connection_pool<Args...>> : std::true_type {};

template <typename T>
constexpr auto ConnectionPool = is_connection_pool<std::decay_t<T>>::value;

/**
 * @brief Connection pool construct helper function
 *
 * Helper function which creates connection pool based on `ConnectionSource` and configuration parameters.
 *
 * @param source --- connection source object which is being used to create connection to a database.
 * @param config --- pool configuration.
 * @param thread_safety --- admissibility to use in multithreaded environment without additional synchronization.
 * Thread safe by default (`ozo::thread_safety<true>`).
 *
 * @return `ozo::connection_pool` object.
 * @ingroup group-connection-functions
 * @relates ozo::connection_pool
 */
template <typename ConnectionSource, typename ThreadSafety = decltype(thread_safe)>
auto make_connection_pool(ConnectionSource&& source, const connection_pool_config& config,
                          const ThreadSafety& thread_safety = ThreadSafety{}) {
    static_assert(ozo::ConnectionSource<ConnectionSource>, "source should model ConnectionSource concept");
    return connection_pool<std::decay_t<ConnectionSource>, std::decay_t<ThreadSafety>>{
        std::forward<ConnectionSource>(source), config, thread_safety};
}

} // namespace ozo

#include <ozo/impl/connection_pool.h>
