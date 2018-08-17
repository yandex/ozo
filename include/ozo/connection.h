#pragma once

#include <ozo/error.h>
#include <ozo/type_traits.h>
#include <ozo/asio.h>
#include <ozo/concept.h>

#include <ozo/detail/bind.h>
#include <ozo/impl/connection.h>

namespace ozo {

using no_statistics = decltype(hana::make_map());

/**
* Overloaded version of unwrap function for Connection.
* Returns connection object itself.
*/
template <typename T>
inline decltype(auto) unwrap_connection(T&& conn,
        Require<!Nullable<T>>* = 0) noexcept {
    return std::forward<T>(conn);
}

/**
* Unwraps wrapped connection recusively.
* Returns unwrapped connection object.
*/
template <typename T>
inline decltype(auto) unwrap_connection(T&& conn,
        Require<Nullable<T>>* = 0) noexcept {
    return unwrap_connection(*conn);
}

template <typename T, typename = std::void_t<>>
struct get_connection_oid_map_impl {
    template <typename Conn>
    constexpr static auto apply(Conn&& c) -> decltype((c.oid_map_)) {
        return c.oid_map_;
    }
};

template <typename T>
constexpr auto get_connection_oid_map(T&& conn)
        -> decltype (get_connection_oid_map_impl<std::decay_t<T>>::apply(std::forward<T>(conn))) {
    return get_connection_oid_map_impl<std::decay_t<T>>::apply(std::forward<T>(conn));
}

template <typename T, typename = std::void_t<>>
struct get_connection_socket_impl {
    template <typename Conn>
    constexpr static auto apply(Conn&& c) -> decltype((c.socket_)) {
        return c.socket_;
    }
};

template <typename T>
constexpr auto get_connection_socket(T&& conn)
        -> decltype(get_connection_socket_impl<std::decay_t<T>>::apply(std::forward<T>(conn))) {
    return get_connection_socket_impl<std::decay_t<T>>::apply(std::forward<T>(conn));
}

template <typename T, typename = std::void_t<>>
struct get_connection_handle_impl {
    template <typename Conn>
    constexpr static auto apply(Conn&& c) -> decltype((c.handle_)) {
        return c.handle_;
    }
};

template <typename T>
constexpr auto get_connection_handle(T&& conn)
        -> decltype(get_connection_handle_impl<std::decay_t<T>>::apply(std::forward<T>(conn))) {
    return get_connection_handle_impl<std::decay_t<T>>::apply(std::forward<T>(conn));
}

template <typename T, typename = std::void_t<>>
struct get_connection_error_context_impl {
    template <typename Conn>
    constexpr static auto apply(Conn&& c) -> decltype((c.error_context_)) {
        return c.error_context_;
    }
};

template <typename T>
constexpr auto get_connection_error_context(T&& conn)
        -> decltype(get_connection_error_context_impl<std::decay_t<T>>::apply(std::forward<T>(conn))) {
    return get_connection_error_context_impl<std::decay_t<T>>::apply(std::forward<T>(conn));
}

template <typename T, typename = std::void_t<>>
struct get_connection_timer_impl {
    template <typename Conn>
    constexpr static auto apply(Conn&& c) -> decltype((c.timer_)) {
        return c.timer_;
    }
};

template <typename T>
constexpr auto get_connection_timer(T&& conn)
        -> decltype(get_connection_timer_impl<std::decay_t<T>>::apply(std::forward<T>(conn))) {
    return get_connection_timer_impl<std::decay_t<T>>::apply(std::forward<T>(conn));
}

template <typename, typename = std::void_t<>>
struct is_connection : std::false_type {};
template <typename T>
struct is_connection<T, std::void_t<
    decltype(get_connection_oid_map(unwrap_connection(std::declval<T&>()))),
    decltype(get_connection_socket(unwrap_connection(std::declval<T&>()))),
    decltype(get_connection_handle(unwrap_connection(std::declval<T&>()))),
    decltype(get_connection_error_context(unwrap_connection(std::declval<T&>()))),
    decltype(get_connection_timer(unwrap_connection(std::declval<T&>()))),
    decltype(get_connection_oid_map(unwrap_connection(std::declval<const T&>()))),
    decltype(get_connection_socket(unwrap_connection(std::declval<const T&>()))),
    decltype(get_connection_handle(unwrap_connection(std::declval<const T&>()))),
    decltype(get_connection_error_context(unwrap_connection(std::declval<const T&>()))),
    decltype(get_connection_timer(unwrap_connection(std::declval<const T&>())))
>> : std::true_type {};


/**
* /breif Connection concept
* We define the Connection to database not as a concrete class or type,
* but as an entity which must support some traits. The reason why we
* choose such strategy is - we want to provide flexibility and extension ability
* for implementation and testing. User can implemet a connection bound additional
* functionality.
*
* This is how connection is determined. Connection is an object for which these
* next free functions are applicable:
*
*   * get_connection_oid_map()
*     Must return reference or proxy for connection's oid map object, which allows
*     to read and modify it. Object must be created via register_types<>() template
*     function or be empty_oid_map in case if no custom types are used with the
*     connection.
*
*   * get_connection_socket()
*     Must return reference or proxy for socket io stream object which allows to bind
*     the connection to the asio io_context, currently boost::asio::posix::stream_descriptor
*     is suppurted only.
*
*   * get_connection_handle()
*     Must return reference or proxy for native_conn_handle object
*
*   * get_connection_error_context()
*     Must return reference or proxy for additional error context. There is no
*     mechanism to privide context depent information via [std|boost]::error_code. So
*     we decide to use connection for this purpose since native libpq error message is
*     bound to connection and all asyncronous operation of the library is bound to
*     connection too. Currently std::string supported as a context object.
*
*   * get_connection_timer()
*     Must return reference or proxy for timer to plan operations cancel by timeout.
*     Should provide boost::asio::basic_waitable_timer like interface.
*/
template <typename T>
constexpr auto Connection = is_connection<std::decay_t<T>>::value;

/**
* Returns PostgreSQL connection handle of the connection.
*/
template <typename T, typename = Require<Connection<T>>>
inline decltype(auto) get_handle(T&& conn) noexcept {
    return get_connection_handle(
        unwrap_connection(std::forward<T>(conn)));
}

/**
* Returns PostgreSQL native connection handle of the connection.
*/
template <typename T, typename = Require<Connection<T>>>
inline decltype(auto) get_native_handle(T&& conn) noexcept {
    return get_handle(std::forward<T>(conn)).get();
}

/**
* Returns socket stream object of the connection.
*/
template <typename T, typename = Require<Connection<T>>>
inline decltype(auto) get_socket(T&& conn) noexcept {
    return get_connection_socket(unwrap_connection(std::forward<T>(conn)));
}

/**
* Returns io_context for connection is bound to.
*/
template <typename T, typename = Require<Connection<T>>>
inline decltype(auto) get_io_context(T& conn) noexcept {
    return get_socket(conn).get_io_service();
}

/**
* Rebinds io_context for the connection
*/
template <typename T, typename IoContext, typename = Require<Connection<T>>>
inline error_code rebind_io_context(T& conn, IoContext& io) {
    using impl::rebind_connection_io_context;
    return rebind_connection_io_context(unwrap_connection(conn), io);
}

/**
* Indicates if connection is bad
*/
template <typename T>
inline Require<Connection<T> && !OperatorNot<T>,
bool> connection_bad(const T& conn) noexcept {
    using impl::connection_status_bad;
    return connection_status_bad(get_native_handle(conn));
}

/**
* Indicates if connection is bad. Overloaded version for
* ConnectionWraper with operator !
*/
template <typename T>
inline Require<Nullable<T> && OperatorNot<T>,
bool> connection_bad(const T& conn) noexcept {
    using impl::connection_status_bad;
    return !conn || connection_status_bad(get_native_handle(conn));
}

/**
* Indicates if connection is not bad
*/
template <typename T, typename = Require<Connection<T>>>
inline bool connection_good(const T& conn) noexcept {
    return !connection_bad(conn);
}

/**
* Returns string view on native libpq connection error
*/
template <typename T, typename = Require<Connection<T>>>
inline std::string_view error_message(T&& conn) {
    return impl::connection_error_message(get_native_handle(conn));
}

/**
* Getter of additional error context. for error occured while operating
* with connection.
*/
template <typename T, typename = Require<Connection<T>>>
inline const auto& get_error_context(const T& conn) {
    return get_connection_error_context(unwrap_connection(conn));
}

/**
* Set the connection error context.
*/
template <typename T, typename Ctx>
inline Require<Connection<T>> set_error_context(T& conn, Ctx&& ctx) {
    get_connection_error_context(unwrap_connection(conn)) = std::forward<Ctx>(ctx);
}

/**
* Reset the connection error context.
*/
template <typename T>
inline Require<Connection<T>> reset_error_context(T& conn) {
    using ctx_type = std::decay_t<decltype(get_error_context(conn))>;
    set_error_context(conn, ctx_type{});
}

/**
* Returns type oids map of the connection
*/
template <typename T, typename = Require<Connection<T>>>
inline decltype(auto) get_oid_map(T&& conn) noexcept {
    return get_connection_oid_map(unwrap_connection(std::forward<T>(conn)));
}

/**
* Returns statistics object of the connection
*/
template <typename T, typename = Require<Connection<T>>>
inline decltype(auto) get_statistics(T&& conn) noexcept {
    return get_connection_statistics(unwrap_connection(std::forward<T>(conn)));
}

/**
* Returns timer for connection operations.
*/
template <typename T, typename = Require<Connection<T>>>
inline decltype(auto) get_timer(T&& conn) noexcept {
    return get_connection_timer(unwrap_connection(std::forward<T>(conn)));
}

template <typename ConnectionProvider, typename = std::void_t<>>
struct get_connection_type {
    using type = typename std::decay_t<ConnectionProvider>::connection_type;
};

template <typename ConnectionProvider>
using connection_type = typename get_connection_type<ConnectionProvider>::type;

template <typename T, typename = std::void_t<>>
struct async_get_connection_impl {
    template <typename Provider, typename Handler>
    static constexpr auto apply(Provider&& p, Handler&& h) ->
        decltype(p.async_get_connection(std::forward<Handler>(h))) {
        p.async_get_connection(std::forward<Handler>(h));
    }
};

template <typename, typename = std::void_t<>>
struct is_connection_source : std::false_type {};

template <typename T>
struct is_connection_source<T, std::void_t<decltype(
    std::declval<T&>()(
        std::declval<io_context&>(),
        std::declval<std::function<void(error_code, connection_type<T>)>>()
    )
)>> : std::true_type {};

template <typename T>
constexpr auto ConnectionSource = is_connection_source<std::decay_t<T>>::value;

template <typename T, typename Handler>
inline auto async_get_connection(T&& provider, Handler&& handler)
         -> decltype(async_get_connection_impl<std::decay_t<T>>::apply(std::forward<T>(provider), std::forward<Handler>(handler))) {
    return async_get_connection_impl<std::decay_t<T>>::apply(std::forward<T>(provider), std::forward<Handler>(handler));
}

template <typename, typename = std::void_t<>>
struct is_connection_provider : std::false_type {};

template <typename T>
struct is_connection_provider<T, std::void_t<decltype(
    async_get_connection(
        std::declval<T&>(),
        std::declval<std::function<void(error_code, connection_type<T>)>>()
    )
)>> : std::true_type {};

template <typename T>
constexpr auto ConnectionProvider = is_connection_provider<std::decay_t<T>>::value;

/**
* Function to get a connection from provider.
* Accepts:
*   provider - connection provider which will be asked for connection
*   token - completion token which determine the continuation of operation
*           it can be callback, yield_context, use_future and other Boost.Asio
*           compatible tokens.
* Returns:
*   completion token depent value like void for callback, connection for the
*   yield_context, std::future<connection> for use_future, and so on.
*/
template <typename T, typename CompletionToken>
inline auto get_connection(T&& provider, CompletionToken&& token) {
    static_assert(ConnectionProvider<T>, "T is not a ConnectionProvider concept");

    using signature_t = void (error_code, connection_type<T>);
    async_completion<CompletionToken, signature_t> init(token);

    async_get_connection(std::forward<T>(provider), init.completion_handler);

    return init.result.get();
}

template <typename T>
struct async_get_connection_impl<T, Require<Connection<T>>> {
    template <typename Conn, typename Handler>
    static constexpr void apply(Conn&& c, Handler&& h) {
        reset_error_context(c);
        decltype(auto) io = get_io_context(c);
        io.dispatch(detail::bind(
            std::forward<Handler>(h), error_code{}, std::forward<Conn>(c)));
    }
};

/**
* Connection is Connection Provider itself, so we need to define connection
* type it provides.
*/
template <typename T>
struct get_connection_type<T, Require<Connection<T>>> {
    using type = std::decay_t<T>;
};

} // namespace ozo
