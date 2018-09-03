#pragma once

#include <ozo/asio.h>
#include <ozo/connection.h>

namespace ozo {

/**
 * @brief Default implementation of ConnectionProvider
 *
 * @ingroup Connection_Provider
 *
 * This is default implementation of the ConnectionProvider concept. It binds
 * `io_context`, additional parameters and ConnectionSource implementation object to
 * bea able provide a Connection via get_connection functionality.
 *
 * @tparam Base --- ConnectionSource implementation
 * @tparam Args --- ConnectionSource additional parameters types.
 */
template <typename Base, typename ... Args>
class connector {
public:
    static_assert(ConnectionSource<Base>, "is not a ConnectionSource");

    /**
     * Source type according to ConnectionProvider requirements
     */
    using source_type = Base;
    /**
     * Connection implementation type according to ConnectionProvider requirements.
     * Specifies the Connection implementation type which can be obtained from this provider.
     */
    using connection_type = typename source_type::connection_type;

    /**
     * @brief Construct a new connector object
     *
     * Constructor as it is.
     *
     * @param base --- ConnectionSource implementation object
     * @param io --- `io_context` for connect IO
     * @param args --- additional arguments to be passed to ConnectionSource to create Connection
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

} // namespace ozo
