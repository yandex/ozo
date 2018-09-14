#pragma once

#include <ozo/impl/connection_pool.h>
#include <ozo/connection_info.h>
#include <ozo/asio.h>

namespace ozo {

/**
 * @brief Connection pool configuration
 * @ingroup group-connection-types
 *
 * Configuration of the ozo::connection_pool, e.g. how many connection are in the pool,
 * how many queries can be in wait queue if all connections are used by another queries,
 * and how long to keep connection open.
 */
struct connection_pool_config {
    std::size_t capacity = 10; //!< maximum number of stored connections
    std::size_t queue_capacity = 128; //!< maximum number of queued requests to get available connection
    time_traits::duration idle_timeout = std::chrono::seconds(60); //!< time interval to close connection after last usage
};

/**
 * @brief Timeouts for the ozo::get_connection() operation
 * @ingroup group-connection-types
 *
 * Time restrictions to get connection from the ozo::connection_pool
 */
struct connection_pool_timeouts {
    time_traits::duration connect = std::chrono::seconds(10); //!< maximum time interval to establish connection with DBMS
    time_traits::duration queue = std::chrono::seconds(10); //!< maximum time interval to wait for available connection handle in conneciton pool
};

/**
 * @brief Connection pool implementation
 * @ingroup group-connection-types
 *
 * This is a simple <a href="https://en.wikipedia.org/wiki/Connection_pool">connection pool</a>
 * implementation which can store up to maximum allowed number of connection to the same database.
 * If all connections are busy client request will be placed into the internal queue to wait for
 * free connection. If where is no free connections but it does not hit the limit of connections
 * the new connection will be created via underlying #ConnectionSource being specified in constructor.
 * All operations are timed out. Connection idle life time is timed out as well. Pool is configurable
 * via constructor and operator().
 *
 * `connection_pool` models #ConnectionSource concept itself using underlying #ConnectionSource.
 *
 * @tparam Source --- underlying #ConnectionSource which is being used to create connection to a database.
 */
template <typename Source>
class connection_pool {
public:
    /**
     * @brief Construct a new connection pool object
     *
     * @param source --- #ConnectionSource object which is being used to create connection to a database.
     * @param config --- pool configuration.
     */
    connection_pool(Source source, const connection_pool_config& config)
    : impl_(config.capacity, config.queue_capacity, config.idle_timeout),
      source_(std::move(source)) {}

    /**
     * @brief Type of connection depends on connection type of Source
     *
     * Type is used to model #ConnectionSource
     */
    using connection_type = impl::pooled_connection_ptr<Source>;

    /**
     * @brief Provides connection is binded to the given io_context
     *
     * In case of success the handler will be called with no `error_code` and
     * established connection object. If no connection available and number of
     * connections of the pool does not hit the limit - a new connection will be made.
     * If pool is out of limits - the handler will be placed into the internal queue to
     * wait for free connection.
     *
     * In case of time-out of connection establishing or being in queue - the handler
     * will be called with error_code.
     *
     * In case of hitting the limit of the internal queue - the handler will be called
     * with error_code.
     *
     * @param io --- `io_context` for the connection IO.
     * @param handler --- a callback with signature `void(error_code, connection_type)`.
     * @param timeouts --- connection acquisition related time-outs
     */
    template <typename Handler>
    void operator ()(io_context& io, Handler&& handler,
            const connection_pool_timeouts& timeouts = connection_pool_timeouts {}) {
        impl_.get_auto_recycle(
            io,
            impl::wrap_pooled_connection_handler(
                io,
                make_connector(source_, io, timeouts.connect),
                std::forward<Handler>(handler)
            ),
            timeouts.queue
        );
    }

    auto stats() const {
        return impl_.stats();
    }

private:
    impl::connection_pool<Source> impl_;
    Source source_;
};

static_assert(ConnectionProvider<connector<connection_pool<connection_info<>>>>, "is not a ConnectionProvider");

template <typename T>
struct is_connection_pool : std::false_type {};

template <typename ...Args>
struct is_connection_pool<connection_pool<Args...>> : std::true_type {};

template <typename T>
constexpr auto ConnectionPool = is_connection_pool<std::decay_t<T>>::value;

/**
 * @brief Connection pool construct helper function
 * @ingroup group-connection-functions
 * @relates ozo::connection_pool
 *
 * @param source --- #ConnectionSource object which is being used to create connection to a database.
 * @param config --- pool configuration.
 * @return connection_pool constructed object
 */
template <typename Source>
auto make_connection_pool(Source&& source, const connection_pool_config& config) {
    static_assert(ConnectionSource<Source>, "is not a ConnectionSource");
    return connection_pool<std::decay_t<Source>>{std::forward<Source>(source), config};
}

} // namespace ozo
