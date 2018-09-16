#pragma once

#include <ozo/impl/connection_pool.h>
#include <ozo/connection_info.h>
#include <ozo/asio.h>

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
};

/**
 * @brief Timeouts for the ozo::get_connection() operation
 * @ingroup group-connection-types
 *
 * Time restrictions to get connection from the `ozo::connection_pool`
 */
struct connection_pool_timeouts {
    time_traits::duration connect = std::chrono::seconds(10); //!< maximum time interval to establish connection with DBMS
    time_traits::duration queue = std::chrono::seconds(10); //!< maximum time interval to wait for available connection handle in `conneciton_pool`
};

/**
 * @brief Connection pool implementation
 * @ingroup group-connection-types
 *
 * This is a simple implementation connection pool (<a href="https://en.wikipedia.org/wiki/Connection_pool">wikipedia</a>).
 * Connection pool allows to store established connections and reuse it to avoid a connect operation for each request.
 * It supports asynchronous request for connections using a queue with optional limits of capacity and wait time.
 * Also connection in the pool may be closed when it is not used for some time - idle timeout.
 *
 * This is how `connection_pool` handles user request to get a #Connection object:
 *
 * * If there is a free connection --- it will be provided for a user immediately.
 * * If all connections are busy but its number less than the limit --- a new connection will be created via the #ConnectionSource and provided for a user.
 * * If all connections are busy and there is no room to create a new one --- the request will be placed into the internal queue to wait for the free connection.
 *
 * The request may be limited by time via optional `connection_pool_timeouts` argument of the `connection_pool::operator()`.
 *
 * `connection_pool` models #ConnectionSource concept itself using underlying #ConnectionSource.
 *
 * @tparam Source --- underlying #ConnectionSource which is being used to create connection to a database.
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
     * @brief Provides connection is binded to the given `io_context`
     *
     * In case of success --- the handler will be invoked as operation succeeded.
     * In case of connection fail, queue timeout or queue full --- the handler will be invoked as operation failed.
     *
     * @param io --- `io_context` for the connection IO.
     * @param handler --- #Handler.
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
 * Helper function which creates connection pool based on #ConnectionSource and configuration parameters.
 *
 * @param source --- #ConnectionSource object which is being used to create connection to a database.
 * @param config --- pool configuration.
 * @return `ozo::connection_pool` object
 */
template <typename Source>
auto make_connection_pool(Source&& source, const connection_pool_config& config) {
    static_assert(ConnectionSource<Source>, "is not a ConnectionSource");
    return connection_pool<std::decay_t<Source>>{std::forward<Source>(source), config};
}

} // namespace ozo
