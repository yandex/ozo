#pragma once

#include <ozo/error.h>
#include <ozo/type_traits.h>
#include <ozo/asio.h>
#include <ozo/concept.h>

#include <ozo/detail/bind.h>
#include <ozo/impl/connection.h>

#include <boost/asio/dispatch.hpp>

namespace ozo {

/**
 * @defgroup group-connection Database connection
 * @brief Database connection related concepts, types and functions.
 */

/**
 * @defgroup group-connection-concepts Concepts
 * @ingroup group-connection
 * @brief Database connection concept definition
 */

using no_statistics = decltype(hana::make_map());

template <typename, typename = std::void_t<>>
struct unwrap_connection_impl {
    template <typename Conn>
    static constexpr decltype(auto) apply(Conn&& conn) noexcept {
        return std::forward<Conn>(conn);
    }
};

/**
 * @ingroup group-connection-functions
 * @brief Unwrap connection if wrapped with Nullable
 *
 * Unwraps wrapped connection recursively. Returns unwrapped connection object.
 *
 * **Customization Point**
 *
 * This is customization point for the Connection enhancement. To customize it
 * it is better to specialize `ozo::unwrap_connection_impl` template for custom type.
 * E.g. such overload is used for the `ozo::impl::pooled_connection` of this library.
 * The deafult implementation of the function is perfect forwarding. And may look like
 * this (*for exposition only - actual implementation may be different*):
 * @code
    template <typename, typename = std::void_t<>>
    struct unwrap_connection_impl {
        template <typename Conn>
        static constexpr decltype(auto) apply(Conn&& conn) noexcept {
            return std::forward<Conn>(conn);
        }
    };
 * @endcode
 * If you are specialize the template for your own parameterized connection wrapper
 * implementation do not forget to call unwrap_connection for the underlying connection type.
 * E.g.:
  * @code
    template <typename T>
    struct unwrap_connection_impl<T, Require<Nullable<T>>>{
        template <typename Conn>
        static constexpr decltype(auto) apply(Conn&& conn) noexcept {
            return unwrap_connection(*conn);
        }
    };
 * @endcode
 * Function overload works as well, but it is safer to specialize the template.
 *
 * @param conn --- wrapped or unwrapped Connection
 * @return unwrapped #Connection object reference
*/
template <typename T>
inline constexpr decltype(auto) unwrap_connection(T&& conn) noexcept {
    return unwrap_connection_impl<std::decay_t<T>>::apply(std::forward<T>(conn));
}

template <typename T>
struct unwrap_connection_impl<T, Require<Nullable<T>>>{
    template <typename Conn>
    static constexpr decltype(auto) apply(Conn&& conn) noexcept {
        return unwrap_connection(*conn);
    }
};

template <typename T, typename = std::void_t<>>
struct get_connection_oid_map_impl {
    template <typename Conn>
    constexpr static auto apply(Conn&& c) -> decltype((c.oid_map_)) {
        return c.oid_map_;
    }
};

/**
 * @ingroup group-connection-functions
 * @brief Get the connection oid map object
 *
 * #Connection types' OID map getter. This function must be specified for a #Connection concept
 * implementation to be conform to. Function must return reference to ozo::oid_map_t template
 * specialization or proxy class object to access connection's types' OID map.
 *
 * **Customization Point**
 *
 * This is customization point for #Connection concept implementation. To customize it please
 * specialize `ozo::get_connection_oid_map_impl` template. Default specialization may look like this
 * (`for exposition only`):
 * @code
    template <typename T, typename = std::void_t<>>
    struct get_connection_oid_map_impl {
        template <typename Conn>
        constexpr static auto apply(Conn&& c) -> decltype((c.oid_map_)) {
            return c.oid_map_;
        }
    };
 * @endcode
 * Function overload works as well, but it is safer to specialize the template.
 * @param conn --- #Connection object
 * @return reference or proxy to OID map object.
 */
template <typename T>
constexpr auto get_connection_oid_map(T&& conn)
#ifdef OZO_DOCUMENTATION
;
#else
        -> decltype (get_connection_oid_map_impl<std::decay_t<T>>::apply(std::forward<T>(conn))) {
    return get_connection_oid_map_impl<std::decay_t<T>>::apply(std::forward<T>(conn));
}
#endif

template <typename T, typename = std::void_t<>>
struct get_connection_socket_impl {
    template <typename Conn>
    constexpr static auto apply(Conn&& c) -> decltype((c.socket_)) {
        return c.socket_;
    }
};

/**
 * @ingroup group-connection-functions
 * @brief Get the connection socket object
 *
 * #Connection socket object getter. #Connection must provide socket for IO. In general it must return
 * reference to an instance of [boost::asio::posix::stream_descriptor](https://www.boost.org/doc/libs/1_66_0/doc/html/boost_asio/reference/posix__stream_descriptor.html).
 *
 * **Customization Point**
 *
 * This is customization point for #Connection concept implementation. To customize it please
 * specialize `ozo::get_connection_socket_impl` template. Default specialization may look like this
 * (`for exposition only`):
 * @code
    template <typename T, typename = std::void_t<>>
    struct get_connection_socket_impl {
        template <typename Conn>
        constexpr static auto apply(Conn&& c) -> decltype((c.socket_)) {
            return c.socket_;
        }
    };
 * @endcode
 * Function overload works as well, but it is safer to specialize the template.
 *
 * @param conn --- #Connection object
 * @return reference or proxy to the socket object
 */
template <typename T>
constexpr auto get_connection_socket(T&& conn)
#ifdef OZO_DOCUMENTATION
;
#else
        -> decltype(get_connection_socket_impl<std::decay_t<T>>::apply(std::forward<T>(conn))) {
    return get_connection_socket_impl<std::decay_t<T>>::apply(std::forward<T>(conn));
}
#endif

template <typename T, typename = std::void_t<>>
struct get_connection_handle_impl {
    template <typename Conn>
    constexpr static auto apply(Conn&& c) -> decltype((c.handle_)) {
        return c.handle_;
    }
};

/**
 * @ingroup group-connection-functions
 * @brief Get the connection handle object
 *
 * PosgreSQL connection handle getter. In general it must return reference or proxy object to
 * `ozo::native_conn_handle` object.
 *
 * **Customization Point**
 *
 * This is customization point for #Connection concept implementation. To customize it please
 * specialize `ozo::get_connection_handle_impl` template. Default specialization may look like this
 * (`for exposition only`):
 * @code
    template <typename T, typename = std::void_t<>>
    struct get_connection_handle_impl {
        template <typename Conn>
        constexpr static auto apply(Conn&& c) -> decltype((c.handle_)) {
            return c.handle_;
        }
    };
 * @endcode
 * Function overload works as well, but it is safer to specialize the template.
 *
 * @param conn --- #Connection object
 * @return reference or proxy object to `ozo::native_conn_handle` object
 */
template <typename T>
constexpr auto get_connection_handle(T&& conn)
#ifdef OZO_DOCUMENTATION
;
#else
        -> decltype(get_connection_handle_impl<std::decay_t<T>>::apply(std::forward<T>(conn))) {
    return get_connection_handle_impl<std::decay_t<T>>::apply(std::forward<T>(conn));
}
#endif

template <typename T, typename = std::void_t<>>
struct get_connection_error_context_impl {
    template <typename Conn>
    constexpr static auto apply(Conn&& c) -> decltype((c.error_context_)) {
        return c.error_context_;
    }
};

/**
 * @ingroup group-connection-functions
 * @brief Get the connection error context object
 *
 * Connection OZO error context getter. In many cases the `error_code` is a good thing, but sometimes
 * you need an additional context which unlucky can not be placed inside an `error_code` object. To
 * solve this problem this additional error context is introduced. Usually it returns a reference to an
 * std::string object. The error context is reset inside get_connection implementation for Connection.
 *
 * **Customization Point**
 *
 * This is customization point for #Connection concept implementation. To customize it please
 * specialize `ozo::get_connection_error_context_impl` template. Default specialization may look like this
 * (`for exposition only`):
 * @code
    template <typename T, typename = std::void_t<>>
    struct get_connection_error_context_impl {
        template <typename Conn>
        constexpr static auto apply(Conn&& c) -> decltype((c.error_context_)) {
            return c.error_context_;
        }
    };
 * @endcode
 * Function overload works as well, but it is safer to specialize the template.
 *
 * @param conn --- #Connection object
 * @return additional error context, for now `std::string` is supported only
 */
template <typename T>
constexpr auto get_connection_error_context(T&& conn)
#ifdef OZO_DOCUMENTATION
;
#else
        -> decltype(get_connection_error_context_impl<std::decay_t<T>>::apply(std::forward<T>(conn))) {
    return get_connection_error_context_impl<std::decay_t<T>>::apply(std::forward<T>(conn));
}
#endif

template <typename T, typename = std::void_t<>>
struct get_connection_timer_impl {
    template <typename Conn>
    constexpr static auto apply(Conn&& c) -> decltype((c.timer_)) {
        return c.timer_;
    }
};

/**
 * @ingroup group-connection-functions
 * @brief Get the connection timer object
 *
 * OZO is all about IO. IO can be for a long time, so almost every user sooner or later
 * needs a timeout for an IO. This library provides such functionality of time-outs. For a time-out
 * implementation the library needs a timer. So this is it. Originally the library expects
 * <a href="https://www.boost.org/doc/libs/1_66_0/doc/html/boost_asio/reference/basic_waitable_timer.html">boost::asio::basic_waitable_timer</a>
 * as a timer interface and uses <a href="https://www.boost.org/doc/libs/1_66_0/doc/html/boost_asio/reference/steady_timer.html">
 * boost::asio::steady_timer</a> as the default implementation. But this place can be customized.
 *
 * **Customization Point**
 *
 * This is customization point for #Connection concept implementation. To customize it please
 * specialize `ozo::get_connection_timer_impl` template. Default specialization may look like this
 * (`for exposition only`):
 * @code
    template <typename T, typename = std::void_t<>>
    struct get_connection_timer_impl {
        template <typename Conn>
        constexpr static auto apply(Conn&& c) -> decltype((c.timer_)) {
            return c.timer_;
        }
    };
 * @endcode
 * Function overload works as well, but it is safer to specialize the template.
 *
 * @param conn --- #Connection object
 * @return timer for the #Connection
 */
template <typename T>
constexpr auto get_connection_timer(T&& conn)
#ifdef OZO_DOCUMENTATION
;
#else
        -> decltype(get_connection_timer_impl<std::decay_t<T>>::apply(std::forward<T>(conn))) {
    return get_connection_timer_impl<std::decay_t<T>>::apply(std::forward<T>(conn));
}
#endif

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
* @ingroup group-connection-concepts
* @brief Database connection concept
*
* We define the `Connection` to database not as a concrete class or type,
* but as an entity which must support some traits. The reason why we
* choose such strategy is - we want to provide flexibility and extension ability
* for implementation and testing. User can implement a connection bound additional
* functionality.
*
* ###Connection Definition
*
* Connection `conn` is an object for which these next statements are valid:
*
* @code
decltype(auto) oid_map = get_connection_oid_map(unwrap_connection(conn));
* @endcode
* There the `oid_map` must be a reference or proxy for connection's oid map object, which allows to read and modify it.
* Object must be created via `ozo::register_types()` template function or be empty_oid_map in case if no custom
* types are used with the connection.
*
* @code
decltype(auto) socket = get_connection_socket(unwrap_connection(conn));
* @endcode
* Must return reference or proxy for a socket IO stream object which allows to bind the connection to the asio
* io_context, currently <a href="https://www.boost.org/doc/libs/1_66_0/doc/html/boost_asio/reference/posix__stream_descriptor.html">boost::asio::posix::stream_descriptor</a> is suppurted only.
*
*
* @code
decltype(auto) handle = get_connection_handle(unwrap_connection(conn));
* @endcode
* Must return reference or proxy for native_conn_handle object
*
* @code
decltype(auto) timer = get_connection_timer(unwrap_connection(conn));
* @endcode
* Must return reference or proxy for timer to plan operations cancel by timeout.
* Should provide <a href="https://www.boost.org/doc/libs/1_66_0/doc/html/boost_asio/reference/basic_waitable_timer.html">boost::asio::basic_waitable_timer</a> like interface.
*
*
* @code
decltype(auto) error_ctx = get_connection_error_context(unwrap_connection(conn));
* @endcode
* Must return reference or proxy for an additional error context.
* @sa ozo::unwrap_connection(),
ozo::get_connection_oid_map(),
ozo::get_connection_socket(),
ozo::get_connection_handle(),
ozo::get_connection_timer(),
ozo::get_connection_error_context()
* @hideinitializer
*/
template <typename T>
constexpr auto Connection = is_connection<std::decay_t<T>>::value;

/**
 * @defgroup group-connection-functions Related functions
 * @ingroup group-connection
 * @brief Connection related functions
 */
///@{
/**
 * @brief PostgreSQL connection handle
 *
 * PostgreSQL native connection handle wrapped with RAII smart pointer.
 * In common cases you do not need this function. It's purpose is to support
 * extention and customization of the library. See get_native_handle for details.
 *
 * @param conn --- #Connection objec
 * @return refernce to a wrapped PostgreSQL connection handle
 */
template <typename T>
inline decltype(auto) get_handle(T&& conn) noexcept {
    static_assert(Connection<T>, "T must be a Connection");
    return get_connection_handle(
        unwrap_connection(std::forward<T>(conn)));
}

/**
 * @brief PostgreSQL native connection handle
 *
 * In common cases you do not need this function.
 * It's purpose is to support extention and customization of the library. So if you
 * want do extend or customize some behaviour via original libpq calls this is what
 * you really need. In other cases this is not the function what you are looking for.
 *
 * @param conn --- #Connection object
 * @return PQConn* --- native handle
 */
template <typename T>
inline decltype(auto) get_native_handle(T&& conn) noexcept {
    static_assert(Connection<T>, "T must be a Connection");
    return get_handle(std::forward<T>(conn)).get();
}

/**
 * @brief Socket stream object of the connection
 *
 * See underlying `ozo::get_connection_socket` for details.
 *
 * @param conn --- #Connection object
 * @return socket stream object of the connection
 */
template <typename T>
inline decltype(auto) get_socket(T&& conn) noexcept {
    static_assert(Connection<T>, "T must be a Connection");
    return get_connection_socket(unwrap_connection(std::forward<T>(conn)));
}

/**
 * @brief IO context for a connection is bound to
 *
 * @param conn --- #Connection object
 * @return `io_context` of socket stream object of the connection
 */
template <typename T>
inline decltype(auto) get_io_context(T& conn) noexcept {
    static_assert(Connection<T>, "T must be a Connection");
    return get_socket(conn).get_io_service();
}

/**
 * @brief Rebinds `io_context` for the connection
 *
 * @param conn --- #Connection which must be rebound
 * @param io --- `io_context` to which conn must be bound
 * @return `error_code` in case of error has been
 */
template <typename T, typename IoContext>
inline error_code rebind_io_context(T& conn, IoContext& io) {
    static_assert(Connection<T>, "T must be a Connection");
    using impl::rebind_connection_io_context;
    return rebind_connection_io_context(unwrap_connection(conn), io);
}

/**
 * @brief Indicates if connection state is bad
 *
 * If conn is #Nullable - checks for null state first via `operator !()`.
 * @param conn --- #Connection object to check
 * @return `true` if connection is in bad or null state, `false` - otherwise.
 */
template <typename T>
inline bool connection_bad(const T& conn) noexcept {
    static_assert(Connection<T>, "T must be a Connection");
    if constexpr (Nullable<T>) {
        if (!conn) {
            return true;
        }
    }
    using impl::connection_status_bad;
    return connection_status_bad(get_native_handle(conn));
}

/**
 * @brief Indicates if connection state is not bad.
 *
 * See `ozo::connection_bad` for details.
 *
 * @param conn --- #Connection to check
 * @return `false` if connection is in bad state, `true` - otherwise
 */
template <typename T>
inline bool connection_good(const T& conn) noexcept {
    static_assert(Connection<T>, "T must be a Connection");
    return !connection_bad(conn);
}

/**
 * @brief Gives native libpq error message
 *
 * Underlying libpq provides additional textual context for different errors which
 * can be while interacting via connection. This function gives access for such messages.
 *
 * @param conn --- #Connection to get message from
 * @return `std::string_view` contains a message
 */
template <typename T>
inline std::string_view error_message(T&& conn) {
    static_assert(Connection<T>, "T must be a Connection");
    return impl::connection_error_message(get_native_handle(conn));
}

/**
 * @brief Gives additional error context from OZO
 *
 * Like libpq OZO provides additional context for different errors which
 * can be while interacting via connection. This function gives access for
 * such context.
 *
 * @sa ozo::set_error_context(), ozo::reset_error_context()
 * @param conn --- #Connection to get context from
 * @return `std::string` contains a context
 */
template <typename T>
inline const auto& get_error_context(const T& conn) {
    static_assert(Connection<T>, "T must be a Connection");
    return get_connection_error_context(unwrap_connection(conn));
}

/**
 * @brief Set connection OZO-related error context
 *
 * Like libpq OZO provides additional context for errors.
 * This function sets such context for the specified connection.
 *
 * @sa ozo::get_error_context(), ozo::reset_error_context()
 * @param conn --- #Connection to set context to
 * @param ctx --- context to set, now only `std::string` is supported
 */
template <typename T, typename Ctx>
inline void set_error_context(T& conn, Ctx&& ctx) {
    static_assert(Connection<T>, "T must be a Connection");
    get_connection_error_context(unwrap_connection(conn)) = std::forward<Ctx>(ctx);
}

/**
 * @brief Reset connection OZO-related error context
 *
 * This function resets OZO-related error context to default.
 *
 * @sa ozo::set_error_context(), ozo::get_error_context()
 * @param conn --- #Connection to reset context to
 */
template <typename T>
inline void reset_error_context(T& conn) {
    static_assert(Connection<T>, "T must be a Connection");
    using ctx_type = std::decay_t<decltype(get_error_context(conn))>;
    set_error_context(conn, ctx_type{});
}

/**
 * @brief Access to a connection OID map
 *
 * This function gives access to a connection OID map, which represents mapping
 * of custom types to its' OIDs in a database connected to.
 *
 * @param conn --- #Connection to access OID map of
 * @return OID map of the Connection
 */
template <typename T>
inline decltype(auto) get_oid_map(T&& conn) noexcept {
    static_assert(Connection<T>, "T must be a Connection");
    return get_connection_oid_map(unwrap_connection(std::forward<T>(conn)));
}

/**
 * @brief Access to a Connection statistics
 *
 * @note This feature is not implemented yet
 *
 * @param conn --- #Connection to access statistics of
 * @return statistics of the Connection
 */
template <typename T>
inline decltype(auto) get_statistics(T&& conn) noexcept {
    static_assert(Connection<T>, "T must be a Connection");
    return get_connection_statistics(unwrap_connection(std::forward<T>(conn)));
}

/**
 * @brief Access to a timer for connection operations.
 *
 * This function is dedicated to a timer access. Since our API provides time-outs for
 * IO operations it needs a timer. After weeks of experiments we decide to keep timer in the
 * Connection. So this function should provide entity with
 * <a href="https://www.boost.org/doc/libs/1_66_0/doc/html/boost_asio/reference/basic_waitable_timer.html">
 * boost::asio::basic_waitable_timer</a>
 * compatible interface (in common cases it is a reference to
 * <a href="https://www.boost.org/doc/libs/1_66_0/doc/html/boost_asio/reference/steady_timer.html">
 * boost::asio::steady_timer</a>).
 *
 * @param conn --- #Connection to access statistics of
 * @return a reference or proxy for a timer
 */
template <typename T>
inline decltype(auto) get_timer(T&& conn) noexcept {
    static_assert(Connection<T>, "T must be a Connection");
    return get_connection_timer(unwrap_connection(std::forward<T>(conn)));
}

///@}
/**
 * @defgroup group-connection-types Types
 * @ingroup group-connection
 * @brief Connection related types.
 */
///@{

/**
 * @brief Connection type getter
 *
 * @ingroup group-connection-types
 *
 * This type describes connection type from a #ConnectionProvider. By default
 * it assumes on ConnectionProvider::connection_type type, like this (`exposition only`):
 *
@code
template <typename ConnectionProvider, typename = std::void_t<>>
struct get_connection_type {
    using type = typename std::decay_t<ConnectionProvider>::connection_type;
};
@endcode
 *
 * **Customization point**
 *
 * Here you can specify how to obtain connection type from your own #ConnectionProvider.
 *
 * @tparam ConnectionProvider
 */
template <typename ConnectionProvider, typename = std::void_t<>>
struct get_connection_type {
    using type = typename std::decay_t<ConnectionProvider>::connection_type;
};

/**
 * @brief Gives exact type of connection which #ConnectionProvider or #ConnectionSource provide
 *
 * @ingroup group-connection-types
 *
 * This type alias can be used to determine exact type of Connection which can be obtained from a
 * ConnectionSource or ConnectionProvider type. It uses get_connection_type to get Connection type.
 *
 * @tparam ConnectionProvider - #ConnectionSource or #ConnectionProvider type.
 */
template <typename ConnectionProvider>
#ifdef OZO_DOCUMENTATION
using connection_type = of connection;
#else
using connection_type = typename get_connection_type<ConnectionProvider>::type;
#endif

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

/**
 * @brief ConnectionSource concept
 *
 * @ingroup group-connection-concepts
 *
 * Before all we need to connect to our database server. First of all we need to know
 * how to connect. Since we are smart enough, we know at least two possible ways - make
 * a connection or get a connection from a pool of ones. How to be? It depends on. But
 * we still need to know how to do it. So, the ConnectionSource is what we need! This entity
 * tell us how to do it. ConnectionSource is a concept of type which can construct and
 * establish connection to a database.
 *
 * ConnectionSource has provide information about #Connection type it constructs. This can
 * be done via `ozo::connection_type` and it's customization.
 *
 * `ConnectionSource` must be a function or a functor with such signature:
 * @code
    void (io_context io, Handler h, SourceRelatedAdditionalArgs&&...);
 * @endcode
 *
 * where Handler must be a functor with such or compatible signature:
 * @code
    void (error_code ec, connection_type<ConnectionSource> conn);
 * @endcode
 *
 * `ConnectionSource` must establish #Connection by means of `io_context` specified as first
 * argument. In case of connection has been established successful must dispatch
 * Handler with empty `error_code` as the first argument and established #Connection as the
 * second one. In case of failure --- error_code with appropriate value and allocated
 * Connection with additional error context if it possible.
 *
 * Basic usage may looks like this:
 * @code
    io_context io;
    //...
    ConnectionSource source;
    //...
    source(io, [](error_code ec, connection_type<ConnectionSource> conn){
    //...
    });
 * @endcode
 *
 * `ConnectionSource` is a part of #ConnectionProvider mechanism and typically it is no needs to
 * use it directly.
 *
 * ###Built-in Connection Sources
 *
 * `ozo::connection_info`, `ozo::connection_pool`.
 *
 * ###Customization point
 *
 * This concept is a customization point for adding or modifying existing connection
 * sources to specify custom behaviour of connection establishing. Have fun and find
 * the best solution you want.
 *
 * @tparam T --- `ConnectionSource` to examine
 * @hideinitializer
 */
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

/**
 * @brief ConnectionProvider concept
 *
 * @ingroup group-connection-concepts
 *
 * `ConnectionProvider` is an entity which binds together #ConnectionSource, `io_context`
 * and allow to get connection by means of get_connection call function.
 *
 * `ConnectionProvider` must provide Connection by means of underlying ConnectionSource.
 * In case of connection has been provided successful `ConnectionProvider` must dispatch
 * Handler with empty error_code as the first argument and provided Connection as the
 * second one. In case of failure --- `error_code` with appropriate value and allocated
 * Connection with additional error context if it possible.
 *
 * Note, #Connection is a `ConnectionProvider` itself and provides self as a #Connection.
 *
 * **Customization point**
 *
 * First of all `ConnectionProvider` must deliver information about connection type it provides.
 * It can be implemented in two ways. By defining a `connection_type` nested type or specializing
 * `ozo::get_connection_type` template.
 *
 * By the second `ozo::get_connection()` needs be supported. It can be done in two ways: by implementing
 * `ozo::async_get_connection` member function, or specializing `ozo::async_get_connection_impl` template.
 *
 * The `ozo::async_get_connection()` member function has to have this signature:
 * @code
    void async_get_connection(Handler&& h);
 * @endcode
 *
 * where Handler must be a functor with such or compatible signature:
 *
 * @code
    void (error_code ec, connection_type<ConnectionProvider> conn);
 * @endcode
 *
 * See `ozo::connector` class - the default implementation and `ozo::get_connection()` description for more details.
 * @tparam T - type to examine.
 * @hideinitializer
 */
template <typename T>
constexpr auto ConnectionProvider = is_connection_provider<std::decay_t<T>>::value;

/**
 * @brief Get a connection from provider
 *
 * @ingroup group-connection-functions
 *
 * Function allows to get #Connection from #ConnectionProvider in asynchronous way. There is
 * built-in customization for Connection which provides connection itself and resets it's
 * error context.
 *
 * **Customization point**
 *
 * This is a customization point of #ConnectionProvider. By default #ConnectionProvider must have
 * `async_get_connection()` member function with signature:
 * @code
    void async_get_connection(Handler&& h);
 * @endcode
 *
 * where Handler must be a functor with such or compatible signature:
 *
 * @code
    void (error_code ec, connection_type<ConnectionProvider> conn);
 * @endcode
 *
 * This behaviour can be customized via `async_get_connection_impl` specialization. E.g. for #Connection
 * possible customization may looks like this (*exposition only*):
 *
 * @code
    template <typename T>
    struct async_get_connection_impl<T, Require<Connection<T>>> {
        template <typename Conn, typename Handler>
        static constexpr void apply(Conn&& c, Handler&& h) {
            reset_error_context(c);
            decltype(auto) io = get_io_context(c);
            asio::dispatch(io, detail::bind(
                std::forward<Handler>(h), error_code{}, std::forward<Conn>(c)));
        }
    };
 * @endcode
 *
 * @param provider --- #ConnectionProvider to get connection from
 * @param token --- completion token which determines the continuation of operation;
 * it can be a callback, a `boost::asio::yield_context`, a `boost::use_future` and other
 * Boost.Asio compatible tokens.
 * @return completion token depent value like void for callback, connection for the
 * `boost::asio::yield_context`, `std::future<Connection>` for `boost::asio::use_future`, and so on.
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
        asio::dispatch(io, detail::bind(std::forward<Handler>(h), error_code{}, std::forward<Conn>(c)));
    }
};

template <typename T>
struct get_connection_type<T, Require<Connection<T>>> {
    using type = std::decay_t<T>;
};

/**
 * @brief Close connection to the database immediately
 *
 * @ingroup group-connection-functions
 *
 * This function closes connection to the database immediately.
 * @note No query cancel operation will be made while closing connection.
 * Use the function with attention - non cancelled query can still produce
 * a work on server-side and consume resources. So it is better to use
 * `ozo::cancel()` function.
 *
 * @param conn --- #Connection to be closed
 */
template <typename T>
inline void close_connection(T&& conn) {
    static_assert(Connection<T>, "T is not a Connection concept");

    error_code ec;
    get_socket(conn).close(ec);
    get_handle(std::forward<T>(conn)).reset();
}

///@}

} // namespace ozo
