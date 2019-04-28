#pragma once

#include <ozo/asio.h>
#include <ozo/connection.h>

namespace ozo {

/**
 * @brief[[DEPRECATED]] default implementation of ConnectionProvider
 *
 * @ingroup group-connection-types
 *
 * @note This version of connector is deprecated, use deadline version without
 *  additional constructor and template arguments instead.
 * @note Deadline version of `connector::async_get_connection()` method do not
 * apply additional arguments to the ConnectionSource.
 *
 * This is the default implementation of the #ConnectionProvider concept. It binds
 * `io_context` and additional #ConnectionSource-specific parameters to the
 * #ConnectionSource implementation object.
 *
 * Thus `connector` can create connection via #ConnectionSource object running its
 * asynchronous connect operation on the `io_context` with additional parameters.
 * As a result, `connector` provides a #Connection object bound to `io_context` via
 * `ozo::get_connection()` function.
 *
 * @tparam Base --- ConnectionSource implementation
 */
template <typename Base, typename ... Args>
class connector{
public:
    static_assert(ConnectionSource<Base>, "is not a ConnectionSource");

    /**
     * Source type according to #ConnectionProvider requirements
     */
    using source_type = Base;
    /**
     * #Connection implementation type according to #ConnectionProvider requirements.
     * Specifies the #Connection implementation type which can be obtained from this provider.
     */
    using connection_type = typename connection_source_traits<source_type>::connection_type;

    /**
     * @brief Construct a new `connector` object
     *
     * @param base --- #ConnectionSource implementation
     * @param io --- `io_context` for asynchronous IO
     * @param args --- additional #ConnectionSource-specific arguments to be passed
     *                 to make a #Connection
     */
    connector(Base& base, io_context& io, std::tuple<Args ...>&& args)
            : base(base), io(io), args(std::move(args)) {
    }

    template <typename Handler>
    void async_get_connection(Handler&& handler) const& {
        std::apply(
            [&] (const auto& ... parameters) {
                base(io, std::forward<Handler>(handler), parameters ...);
            },
            args
        );
    }

    template <typename Handler>
    void async_get_connection(Handler&& handler) & {
        std::apply(
            [&] (auto& ... parameters) {
                base(io, std::forward<Handler>(handler), parameters ...);
            },
            args
        );
    }

    template <typename Handler>
    void async_get_connection(Handler&& handler) && {
        std::apply(
            [&] (auto&& ... parameters) {
                base(io, std::forward<Handler>(handler), parameters ...);
            },
            std::move(args)
        );
    }

    template <typename Handler>
    void async_get_connection(deadline at, Handler&& handler) const& {
        base(io, at, std::forward<Handler>(handler));
    }

    template <typename Handler>
    void async_get_connection(deadline at, Handler&& handler) & {
        base(io, at, std::forward<Handler>(handler));
    }

    template <typename Handler>
    void async_get_connection(deadline at, Handler&& handler) && {
        std::move(base)(io, at, std::forward<Handler>(handler));
    }
private:
    Base& base;
    io_context& io;
    std::tuple<Args ...> args;
};

template <class Base, class ... Args>
auto make_connector(Base& base, io_context& io, Args&& ... args) {
    return connector<std::decay_t<Base>, std::decay_t<Args> ...>(
        base,
        io,
        std::make_tuple(std::forward<Args>(args) ...)
    );
}

template <class Base, class ... Args>
auto make_connector(const Base& base, io_context& io, Args&& ... args) {
    return connector<const std::decay_t<Base>, std::decay_t<Args> ...>(
        base,
        io,
        std::make_tuple(std::forward<Args>(args) ...)
    );
}

/**
 * @brief Default implementation of ConnectionProvider
 *
 * @ingroup group-connection-types
 *
 * This is the default implementation of the #ConnectionProvider concept. It binds
 * `io_context` to the #ConnectionSource implementation object.
 *
 * Thus `connection_provider` can create connection via #ConnectionSource object running its
 * asynchronous connect operation on the `io_context` with additional parameters.
 * As a result, `connection_provider` provides a #Connection object bound to `io_context` via
 * `ozo::get_connection()` function.
 *
 * @tparam Source --- #ConnectionSource implementation
 */
template <typename Source>
class connection_provider {
public:
    static_assert(ConnectionSource<Source>, "is not a ConnectionSource");

    /**
     * Source type according to #ConnectionProvider requirements
     */
    using source_type = Source;
    /**
     * #Connection implementation type according to #ConnectionProvider requirements.
     * Specifies the #Connection implementation type which can be obtained from this provider.
     */
    using connection_type = typename connection_source_traits<source_type>::connection_type;

    /**
     * @brief Construct a new `connection_provider` object
     *
     * @param source --- #ConnectionSource implementation
     * @param io --- `io_context` for asynchronous IO
     */
    template <typename T>
    connection_provider(T&& source, io_context& io)
     : source_(std::forward<T>(source)), io_(io) {
    }

    template <typename Handler>
    void async_get_connection(deadline at, Handler&& handler) const& {
        source_(io_, at, std::forward<Handler>(handler));
    }

    template <typename Handler>
    void async_get_connection(deadline at, Handler&& handler) & {
        source_(io_, at, std::forward<Handler>(handler));
    }

    template <typename Handler>
    void async_get_connection(deadline at, Handler&& handler) && {
        std::forward<Source>(source_)(io_, at, std::forward<Handler>(handler));
    }

    template <typename Handler>
    void async_get_connection(Handler&& handler) const& {
        source_(io_, std::forward<Handler>(handler));
    }

    template <typename Handler>
    void async_get_connection(Handler&& handler) & {
        source_(io_, std::forward<Handler>(handler));
    }

    template <typename Handler>
    void async_get_connection(Handler&& handler) && {
        std::forward<Source>(source_)(io_, std::forward<Handler>(handler));
    }

private:
    source_type source_;
    io_context& io_;
};

template <typename T>
connection_provider(T&& source, io_context& io) -> connection_provider<T>;

} // namespace ozo
