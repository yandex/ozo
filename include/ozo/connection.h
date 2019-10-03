#pragma once

#include <ozo/error.h>
#include <ozo/type_traits.h>
#include <ozo/asio.h>
#include <ozo/core/concept.h>
#include <ozo/core/recursive.h>
#include <ozo/core/none.h>
#include <ozo/deadline.h>
#include <ozo/native_conn_handle.h>

#include <ozo/detail/bind.h>
#include <ozo/detail/functional.h>

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

template <typename T>
struct unwrap_connection_impl : unwrap_recursive_impl<T> {};
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
    template <typename T>
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
    struct unwrap_connection_impl<CustomWrapper<T>>{
        template <typename Conn>
        static constexpr decltype(auto) apply(Conn&& conn) noexcept {
            return unwrap_connection(conn.underlying());
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
    return detail::apply<unwrap_connection_impl>(std::forward<T>(conn));
}

template <typename, typename = std::void_t<>>
struct is_connection : std::false_type {};

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
* @code{cpp}
decltype(auto) oid_map = get_connection_oid_map(unwrap_connection(conn));
* @endcode
* There the `oid_map` must be a reference or proxy for connection's #OidMap object, which allows to read and modify it.
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
* Must return reference or proxy for native_conn_handle object.
*
*
* @code
decltype(auto) error_ctx = get_connection_error_context(unwrap_connection(conn));
* @endcode
* Must return reference or proxy for an additional error context.
*
*
* @code
decltype(auto) ex = get_connection_executor(unwrap_connection(conn));
* @endcode
* Should return IO operations executor associated with connection.
* @sa ozo::unwrap_connection(),
ozo::get_connection_oid_map(),
ozo::get_connection_socket(),
ozo::get_connection_handle(),
ozo::get_connection_error_context(),
ozo::get_connection_executor()
* @hideinitializer
*/
template <typename T>
constexpr auto Connection = is_connection<std::decay_t<decltype(unwrap_connection(std::declval<T>()))>>::value;

/**
 * @defgroup group-connection-functions Related functions
 * @ingroup group-connection
 * @brief Connection related functions
 */
///@{

/**
 * @brief PostgreSQL native connection handle
 *
 * In common cases you do not need this function.
 * It's purpose is to support extention and customization of the library. So if you
 * want do extend or customize some behaviour via original libpq calls this is what
 * you really need. In other cases this is not the function what you are looking for.
 *
 * @param conn --- #Connection object
 * @return native handle
 */
template <typename Connection>
inline auto get_native_handle(const Connection& conn) noexcept;

/**
 * @brief Executor for connection related asynchronous operations
 *
 * This executor must be used to schedule all the asynchronous operations
 * related to #Connection to allow timer-based timeouts works for all of the
 * operations.
 *
 * @param conn --- #Connection object
 * @return `executor` of socket stream object of the connection
 */
template <typename T>
inline auto get_executor(T& conn) noexcept;

/**
 * @brief Binds executor for the connection
 *
 * @param conn --- #Connection which must be rebound
 * @param ex --- Executor whish should be used for IO and timers
 * @return `error_code` in case of error has been
 */
template <typename T, typename Executor>
inline error_code bind_executor(T& conn, const Executor& ex);

/**
 * @brief Indicates if connection state is bad
 *
 * If conn is #Nullable - checks for null state first via `operator !()`.
 * @param conn --- #Connection object to check
 * @return `true` if connection is in bad or null state, `false` - otherwise.
 */
template <typename T>
inline bool connection_bad(const T& conn) noexcept;

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
inline std::string_view error_message(T&& conn);

/**
 * @brief Additional error context getter
 *
 * In addition to libpq OZO provides its own error context. This is
 * a getter for such a context. Please be sure that the connection
 * is not in the null state via `ozo::is_null_recursive()` function.
 *
 * @sa ozo::set_error_context(), ozo::reset_error_context(), ozo::is_null_recursive()
 * @param conn --- #Connection to get context from, should not be in null state
 * @return `std::string` contains a context
 */
template <typename T>
inline const auto& get_error_context(const T& conn);

/**
 * @brief Additional error context setter
 *
 * In addition to libpq OZO provides its own error context. This is
 * a setter for such a context. Please be sure that the connection
 * is not in the null state via `ozo::is_null_recursive()` function.
 *
 * @sa ozo::get_error_context(), ozo::reset_error_context(), ozo::is_null_recursive()
 * @param conn --- #Connection to set context to
 * @param ctx --- context to set, now only `std::string` is supported
 */
template <typename T, typename Ctx>
inline void set_error_context(T& conn, Ctx&& ctx);

/**
 * @brief Reset additional error context
 *
 * This function resets additional error context to its default value.
 * Please be sure that the connection  is not in the null state via
 * `ozo::is_null_recursive()` function.
 *
 * @sa ozo::set_error_context(), ozo::get_error_context(), ozo::is_null_recursive()
 * @param conn --- #Connection to reset context, should not be in null state
 */
template <typename T>
inline void reset_error_context(T& conn);

/**
 * @brief Access to a connection OID map
 *
 * This function gives access to a connection OID map, which represents mapping
 * of custom types to its' OIDs in a database connected to.
 * Please be sure that the connection  is not in the null state via
 * `ozo::is_null_recursive()` function.
 *
 * @param conn --- #Connection to access OID map, should not be in null state
 * @return OID map of the Connection
 */
template <typename T>
inline decltype(auto) get_oid_map(T&& conn) noexcept;

/**
 * @brief Access to a Connection statistics
 *
 * @note This feature is not implemented yet
 *
 * Please be sure that the connection  is not in the null state via
 * `ozo::is_null_recursive()` function.
 *
 * @param conn --- #Connection to access statistics, should not be in null state
 * @return statistics of the Connection
 */
template <typename T>
inline decltype(auto) get_statistics(T&& conn) noexcept;

/**
 * @brief Get the database name of the active connection
 *
 * See documentation for the underlying [PQdb](https://www.postgresql.org/docs/current/libpq-status.html)
 * function for additional information.
 *
 * @note Connection should not be is null recursive.
 *
 * @param conn --- active connection to a database.
 * @return std::string_view --- string view with database name.
 */
template <typename Connection>
inline std::string_view get_database(const Connection& conn);

/**
 * @brief Get the host connected of the active connection
 *
 * See documentation for the underlying [PQhost](https://www.postgresql.org/docs/current/libpq-status.html)
 * function for additional information.
 *
 * @note Connection should not be is null recursive.
 *
 * @param conn --- active connection to a database.
 * @return std::string_view --- string view with host.
 */
template <typename Connection>
inline std::string_view get_host(const Connection& conn);

/**
 * @brief Get the port connected of the active connection
 *
 * See documentation for the underlying [PQport](https://www.postgresql.org/docs/current/libpq-status.html)
 * function for additional information.
 *
 * @note Connection should not be is null recursive.
 *
 * @param conn --- active connection to a database.
 * @return std::string_view --- string view with port.
 */
template <typename Connection>
inline std::string_view get_port(const Connection& conn);

/**
 * @brief Get the user name of the active connection
 *
 * See documentation for the underlying [PQuser](https://www.postgresql.org/docs/current/libpq-status.html)
 * function for additional information.
 *
 * @note Connection should not be is null recursive.
 *
 * @param conn --- active connection to a database.
 * @return std::string_view --- string view with user name.
 */
template <typename Connection>
inline std::string_view get_user(const Connection& conn) ;

/**
 * @brief Get the password of the active connection
 *
 * See documentation for the underlying [PQpass](https://www.postgresql.org/docs/current/libpq-status.html)
 * function for additional information.
 *
 * @note Connection should not be is null recursive.
 *
 * @param conn --- active connection to a database.
 * @return std::string_view --- string view with password.
 */
template <typename Connection>
inline std::string_view get_password(const Connection& conn);
///@}
/**
 * @defgroup group-connection-types Types
 * @ingroup group-connection
 * @brief Connection related types.
 */
///@{

namespace detail {

template <typename ConnectionProvider, typename = std::void_t<>>
struct get_connection_type_default {};

template <typename ConnectionProvider>
struct get_connection_type_default<ConnectionProvider,
    std::void_t<typename ConnectionProvider::connection_type>> {
    using type = typename ConnectionProvider::connection_type;
};

} // namespace detail

/**
 * @brief Connection type getter
 *
 * This type describes connection type from a #ConnectionProvider.
 *
 * @tparam ConnectionProvider - #ConnectionProvider type to get #Connection type from.
 *
 * By default it assumes on `ConnectionProvider::connection_type` type, if no nested type
 * `connection_type` is found no inner type `type`.
 * Possible implementation may look like this (`exposition only`):
 *
@code
template <typename T, typename = void>
struct get_connection_type_default {};

template <typename T>
struct get_connection_type_default<T,
        std::void_t<typename T::connection_type> {
    using type = typename T::connection_type;
};

template <typename T>
struct get_connection_type : get_connection_type_default<T>{};
@endcode
 *
 * ### Customization point
 *
 * Here you can specify how to obtain #Connection type from your own #ConnectionProvider.
 * E.g. for custom #Connection implementation which is used via pointer the specialization
 * can be:
@code
template <>
struct get_connection_type<MyConnection*> {
    using type = MyConnection*;
};
@endcode
 *
 * @ingroup group-connection-types
 */
template <typename ConnectionProvider>
struct get_connection_type
#ifdef OZO_DOCUMENTATION
{
    using type = <implementation defined>; ///!< Type of #Connection object provided by the #ConnectionProvider specified
};
#else
 : detail::get_connection_type_default<ConnectionProvider>{};
#endif

/**
 * @brief Gives exact type of a connection object which #ConnectionProvider or #ConnectionSource provide
 *
 * This type alias can be used to determine exact type of a #Connection object which can be obtained from a
 * #ConnectionSource or #ConnectionProvider. It uses `ozo::get_connection_type` metafunction
 * to get a #Connection type.
 *
 * @tparam ConnectionProvider - #ConnectionSource or #ConnectionProvider type.
 * @ingroup group-connection-types
 */
template <typename ConnectionProvider>
using connection_type = typename get_connection_type<std::decay_t<ConnectionProvider>>::type;

template <typename T>
using handler_signature = void (error_code, connection_type<T>);

namespace detail {

struct call_async_get_connection {
    template <typename Provider, typename TimeConstraint, typename Handler>
    static constexpr auto apply(Provider&& p, TimeConstraint t, Handler&& h) ->
            decltype(p.async_get_connection(t, std::forward<Handler>(h))) {
        static_assert(ozo::TimeConstraint<TimeConstraint>, "should model TimeConstraint concept");
        return p.async_get_connection(t, std::forward<Handler>(h));
    }
};

struct forward_connection {
    template <typename Conn, typename TimeConstraint, typename Handler>
    static constexpr void apply(Conn&& c, TimeConstraint, Handler&& h) {
        static_assert(ozo::TimeConstraint<TimeConstraint>, "should model TimeConstraint concept");
        reset_error_context(c);
        auto ex = get_executor(c);
        asio::dispatch(ex, detail::bind(std::forward<Handler>(h), error_code{}, std::forward<Conn>(c)));
    }
};

} // namespace detail

template <typename Provider, typename TimeConstraint>
struct async_get_connection_impl : std::conditional_t<Connection<Provider>,
    detail::forward_connection,
    detail::call_async_get_connection
> {};

namespace detail {

template <typename Source, typename TimeTraits, typename = std::void_t<>>
struct connection_source_supports_time_traits : std::false_type {};

template <typename Source, typename TimeTraits>
struct connection_source_supports_time_traits<Source, TimeTraits, std::void_t<decltype(
    std::declval<Source&>()(
        std::declval<io_context&>(),
        std::declval<TimeTraits>(),
        std::declval<handler_signature<Source>>()
    )
)>> : std::true_type {};

template <typename T>
using connection_source_defined = std::conjunction<
    typename connection_source_supports_time_traits<T, time_traits::time_point>::type,
    typename connection_source_supports_time_traits<T, time_traits::duration>::type,
    typename connection_source_supports_time_traits<T, none_t>::type
>;
} // namespace detail

template <typename T>
using is_connection_source = typename detail::connection_source_defined<T>::type;

template <typename T>
struct connection_source_traits {
    using type = connection_source_traits;
    using connection_type = typename get_connection_type<std::decay_t<T>>::type;
};

/**
 * @brief ConnectionSource concept
 *
 * Before all we need to connect to our database server. First of all we need to know
 * how to connect. Since we are smart enough, we know at least two possible ways - make
 * a connection or get a connection from a pool of ones. How to be? It depends on. But
 * we still need to know how to do it. So, the `ConnectionSource` is what we need! This entity
 * tell us how to do it. ConnectionSource is a concept of type which can construct and
 * establish connection to a database.
 *
 * ConnectionSource has provide information about #Connection type it constructs. This can
 * be done via `ozo::connection_type` and it's customization.
 *
 * `ConnectionSource` should be a callable object with such signature:
 * @code
    void (io_context& io, TimeConstarint t, Handler&& h);
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
    using std::chrono_literals;
    source(io, 1s, [](error_code ec, connection_type<ConnectionSource> conn){
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
 * @ingroup group-connection-concepts
 */
template <typename T>
constexpr auto ConnectionSource = is_connection_source<std::decay_t<T>>::value;

template <typename Provider, typename TimeConstraint, typename Handler>
constexpr auto async_get_connection(Provider&& p, TimeConstraint t, Handler&& h) ->
        decltype(async_get_connection_impl<std::decay_t<Provider>, TimeConstraint>::
            apply(std::forward<Provider>(p), t, std::forward<Handler>(h))) {
    static_assert(ozo::TimeConstraint<TimeConstraint>, "should model TimeConstraint concept");
    return async_get_connection_impl<std::decay_t<Provider>, TimeConstraint>::
            apply(std::forward<Provider>(p), t, std::forward<Handler>(h));
}

namespace detail {
template <typename Provider, typename TimeConstraint, typename = std::void_t<>>
struct connection_provider_supports_time_constraint : std::false_type {};

template <typename Provider, typename TimeConstraint>
struct connection_provider_supports_time_constraint<Provider, TimeConstraint, std::void_t<decltype(
    async_get_connection(
        std::declval<Provider&>(),
        std::declval<TimeConstraint>(),
        std::declval<handler_signature<Provider>>()
    )
)>> : std::true_type {};

template <typename T>
using async_get_connection_defined = std::conjunction<
    typename connection_provider_supports_time_constraint<T, none_t>::type,
    typename connection_provider_supports_time_constraint<T, time_traits::duration>::type,
    typename connection_provider_supports_time_constraint<T, time_traits::time_point>::type
>;

} // namespace detail

template <typename T>
using is_connection_provider = typename detail::async_get_connection_defined<T>::type;

template <typename T>
struct connection_provider_traits {
    using type = connection_provider_traits;
    using connection_type = typename get_connection_type<std::decay_t<T>>::type;
};

/**
 * @brief ConnectionProvider concept
 *
 * `ConnectionProvider` is an entity which provides ready-to-use #Connection by means of
 * `ozo::get_connection()` function call.
 *
 * `ConnectionProvider` may provide #Connection via the #ConnectionSource
 * (see `ozo::connection_provider` as an example of such entity).
 * In case of #Connection has been provided successful `ConnectionProvider` should dispatch
 * #Handler with empty error_code as the first argument and the #Connection as the
 * second one. In case of failure --- `ozo::error_code` with appropriate value and allocated
 * #Connection with additional error context, but only if it possible. Overwise #Connection
 * should be in null state
 *
 * @note #Connection is a `ConnectionProvider` itself.
 *
 * ###Customization point
 *
 * Typically it is enough to customize #ConnectionSource, but sometimes it is more convenient
 * to make a custom #ConnectionProvider, e.g. a user has a structure is binded to certain
 * `ozo::io_context`. So in this case these steps should be done.
 *
 * * `ConnectionProvider` should deliver an information about connection type it provides.
 * It may be implemented via defining a `connection_type` nested type or specializing
 * `ozo::get_connection_type` template. See `ozo::get_connection_type` for details.
 *
 * * `ozo::get_connection()` needs to be supported. See `ozo::get_connection()` for details.
 *
 * See `ozo::connection_provider` class - the default implementation and `ozo::get_connection()` description for more details.
 * @tparam T - type to examine.
 * @hideinitializer
 * @ingroup group-connection-concepts
 */
template <typename T>
constexpr auto ConnectionProvider = is_connection_provider<std::decay_t<T>>::value;

#ifdef OZO_DOCUMENTATION
/**
 * @brief Get a #Connection from #ConnectionProvider
 *
 * Retrives #Connection from #ConnectionProvider. There is built-in customization
 * for #Connection which provides connection itself and resets it's error context.
 *
 * @note The function does not particitate in ADL since could be implemented via functional object.
 *
 * @param provider --- #ConnectionProvider to get connection from.
 * @param time_constraint --- #TimeConstraint for the operation.
 * @param token --- operation #CompletionToken.
 * @return deduced from #CompletionToken.
 *
 * ###Customization point
 *
 * This is a customization point of #ConnectionProvider. By default #ConnectionProvider should have
 * `async_get_connection()` member function with signature:
 * @code
    template <typename TimeConstraint>
    void async_get_connection(TimeConstraint t, Handler&& h);
 * @endcode
 *
 * This behaviour can be customized via `async_get_connection_impl` specialization. E.g. for custom implementation
 * of #Connection customization may looks like this (*exposition only*):
 *
 * @code
    template <typename TimeConstrain, typename ...Ts>
    struct async_get_connection_impl<MyConnection<Ts...>, TimeConstrain> {
        template <typename Conn, typename Handler>
        static constexpr void apply(Conn&& c, TimeConstraint t, Handler&& h) {
            c->prepareForNextOp();
            async_get_connection_impl_default<MyConnection<Ts...>, TimeConstrain>::apply(
                std::forward<Conn>(c), t, std::forward<Handler>(h)
            );
        }
    };
 * @endcode
 * @ingroup group-connection-functions
 */
template <typename T, typename TimeConstraint, typename CompletionToken>
decltype(auto) get_connection(T&& provider, TimeConstraint time_constraint, CompletionToken&& token);
/**
 * @brief Get a connection from provider with no time constains
 *
 * This function is time constrain free shortcut to `ozo::get_connection()` function.
 * Its call is equal to `ozo::get_connection(provider, ozo::none, token)` call.
 *
 * @note The function does not particitate in ADL since could be implemented via functional object.
 *
 * @param provider --- #ConnectionProvider to get connection from.
 * @param token --- operation #CompletionToken.
 * @return deduced from #CompletionToken.
 * @ingroup group-connection-functions
 */
template <typename T, typename CompletionToken>
decltype(auto) get_connection(T&& provider, CompletionToken&& token);
#else
template <typename Initiator>
struct get_connection_op : base_async_operation <get_connection_op<Initiator>, Initiator> {
    using base = typename get_connection_op::base;
    using base::base;

    template <typename T, typename TimeConstraint, typename CompletionToken>
    decltype(auto) operator() (T&& provider, TimeConstraint t, CompletionToken&& token) const {
        static_assert(ConnectionProvider<T>, "T is not a ConnectionProvider concept");
        static_assert(ozo::TimeConstraint<TimeConstraint>, "should model TimeConstraint concept");
        return async_initiate<CompletionToken, handler_signature<T>>(
            get_operation_initiator(*this), token, std::forward<T>(provider), t
        );
    }

    template <typename T, typename CompletionToken>
    decltype(auto) operator() (T&& provider, CompletionToken&& token) const {
        return (*this)(std::forward<T>(provider), none, std::forward<CompletionToken>(token));
    }

    template <typename OtherInitiator>
    constexpr static auto rebind_initiator(const OtherInitiator& other) {
        return get_connection_op<OtherInitiator>{other};
    }
};

namespace detail {
struct initiate_async_get_connection {
    template <typename Provider, typename Handler, typename TimeConstraint>
    constexpr void operator()(Handler&& h, Provider&& p, TimeConstraint t) const {
        async_get_connection( std::forward<Provider>(p), t, std::forward<Handler>(h));
    }
};
} // namespace detail

constexpr get_connection_op<detail::initiate_async_get_connection> get_connection;
#endif

namespace detail {

template <typename T>
struct get_connection_type_default<T, Require<Connection<T>>> {
    using type = T;
};

} // namespace detail

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
template <typename Connection>
inline error_code close_connection(Connection&& conn);

/**
 * @brief Close connection to the database when leaving the scope
 *
 * This function creates RAII guard object which calls `ozo::close_connection()`
 * at the end of its scope. If nullptr is passed as argument or connection is
 * null recursive no `ozo::close_connection()` would be made.
 *
 * @param conn --- pointer on a #Connection to be closed.
 *
 * ###Example
 *
 * @code
{
    auto guard = defer_close_connection(should_be_closed ? std::addressof(conn) : nullptr);

    // Process the connection
}
 * @endcode
 *
 * @ingroup group-connection-functions
 */
template <typename Connection>
inline auto defer_close_connection(Connection* conn) {
    static_assert(ozo::Connection<Connection>, "argument should model Connection");

    auto do_close = [] (auto conn_ptr) {
        if (!is_null_recursive(*conn_ptr)) {
            close_connection(*conn_ptr);
        }
    };

    return std::unique_ptr<Connection, decltype(do_close)>{conn, do_close};
}

///@}

} // namespace ozo

#include <ozo/impl/connection.h>
