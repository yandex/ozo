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
#include <boost/asio/posix/stream_descriptor.hpp>

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

template <typename T, typename = std::void_t<>>
struct get_connection_oid_map_impl {
    template <typename Conn>
    constexpr static auto apply(Conn&& c) -> decltype((c.oid_map_)) {
        return c.oid_map_;
    }
};

template <typename T>
constexpr detail::result_of<get_connection_oid_map_impl, T> get_connection_oid_map(T&& conn) {
    return detail::apply<get_connection_oid_map_impl>(std::forward<T>(conn));
}

template <typename T, typename = std::void_t<>>
struct get_connection_socket_impl {
    template <typename Conn>
    constexpr static auto apply(Conn&& c) -> decltype((c.socket_)) {
        return c.socket_;
    }
};
template <typename T>
constexpr detail::result_of<get_connection_socket_impl, T> get_connection_socket(T&& conn) {
    return detail::apply<get_connection_socket_impl>(std::forward<T>(conn));
}

template <typename T, typename = hana::when<true>>
struct get_connection_executor_impl;

template <typename T>
struct get_connection_executor_impl<T,
    hana::when_valid<decltype(std::declval<const T&>().get_executor())>
> {
    constexpr static auto apply(const T& c) {
        return c.get_executor();
    }
};

template <typename T>
constexpr detail::result_of<get_connection_executor_impl, T> get_connection_executor(T&& conn) {
    return detail::apply<get_connection_executor_impl>(std::forward<T>(conn));
}

template <typename T, typename = std::void_t<>>
struct get_connection_handle_impl {
    template <typename Conn>
    constexpr static auto apply(Conn&& c) -> decltype((c.handle_)) {
        return c.handle_;
    }
};

template <typename T>
constexpr detail::result_of<get_connection_handle_impl, T> get_connection_handle(T&& conn) {
    return detail::apply<get_connection_handle_impl>(std::forward<T>(conn));
}

template <typename T, typename = std::void_t<>>
struct get_connection_error_context_impl {
    template <typename Conn>
    constexpr static auto apply(Conn&& c) -> decltype((c.error_context_)) {
        return c.error_context_;
    }
};

template <typename T>
constexpr detail::result_of<get_connection_error_context_impl, T> get_connection_error_context(T&& conn) {
    return detail::apply<get_connection_error_context_impl>(std::forward<T>(conn));
}

namespace detail {
template <typename Connection, typename Executor>
inline error_code bind_connection_executor(Connection&, const Executor&);
}

/**
 * @brief Default model for `Connection` concept
 *
 * `Connection` concept model which is used by the library as default model.
 *
 * @tparam OidMap --- oid map of types are used with connection
 * @tparam Statistics --- statistics of the connection (not supported yet)
 *
 * ### Thread safety
 *
 * *Distinct objects*: Safe.
 *
 * *Shared objects*: Unsafe.
 *
 * @ingroup group-connection-types
 */
template <typename OidMap, typename Statistics>
class connection {
public:
    using native_handle_type = native_conn_handle::pointer; //!< Native connection handle type
    using oid_map_type = OidMap; //!< Oid map of types that are used with the connection
    using error_context = std::string; //!< Additional error context which could provide context depended information for errors
    using executor_type = io_context::executor_type; //!< The type of the executor associated with the object.

    /**
     * Construct a new connection object.
     *
     * @param io --- execution context for IO operations associated with the object.
     * @param statistics --- initial statistics (not supported yet)
     */
    connection(io_context& io, Statistics statistics);

    /**
     * Get native connection handle object.
     *
     * This function may be used to obtain the underlying representation of the connection.
     * This is intended to allow access to native `libpq` functionality that is not otherwise provided.
     *
     * @return native_handle_type --- native connection handle.
     */
    native_handle_type native_handle() const noexcept { return handle_.get(); }

    /**
     * Get a reference to an oid map object for types that are used with the connection.
     *
     * @return oid_map_type& --- reference on oid map object.
     */
    oid_map_type& oid_map() noexcept { return oid_map_;}
    /**
     * Get a reference to an oid map object for types that are used with the connection.
     *
     * @return const oid_map_type& --- reference on oid map object.
     */
    const oid_map_type& oid_map() const noexcept { return oid_map_;}

    Statistics& statistics() noexcept { return statistics_;}
    const Statistics& statistics() const noexcept { return statistics_;}

    /**
     * Get the additional context object for an error that occurred during the last operation on the connection.
     *
     * @return const error_context& --- additional context for the error
     */
    const error_context& get_error_context() const noexcept { return error_context_; }
    /**
     * Set the additional error context object. This function may be used to provide additional context-depended
     * data that is related to the current operation error.
     *
     * @param v --- new error context.
     */
    void set_error_context(error_context v = error_context{}) { error_context_ = std::move(v); }

    /**
     * Get the executor associated with the object.
     *
     * @return executor_type --- executor object.
     */
    executor_type get_executor() const noexcept { return io_->get_executor(); }

    /**
     * Set the new executor for the object.
     *
     * This function may be used to migrate the object between different execution contexts.
     *
     * Typically this function is used by the library in the connection pool implementation.
     * Users should not use it directly other than for a special purpose (e.g., own connection pool
     * implementation and so on).
     *
     * @note The function shall not be called while any active operation executes on the object.
     *
     * @param ex --- new executor object.
     * @return error_code --- error code of the function call.
     */
    error_code set_executor(const executor_type& ex);

    /**
     * Assign an existing native connection handle to the object.
     *
     * Typically this function is used by the library within the connection establishing process.
     * Users should not use it directly other than for a special purpose (e.g., special connection pool
     * implementation and so on).
     *
     * @note The function shall not be called while any active operation executes on the object.
     *
     * @param handle --- rvalue reference on a new handle.
     * @return error_code --- error code of the function call.
     */
    error_code assign(native_conn_handle&& handle);

    /**
     * Asynchronously wait for the connection socket to become ready to write or to have pending error conditions.
     *
     * Typically this function is used by the library within the connection establishing process and operation execution.
     * Users should not use it directly other than for custom `libpq`-based opeartions.
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

private:
    using stream_type = asio::posix::stream_descriptor;

    template <typename Connection, typename Executor>
    friend error_code ozo::detail::bind_connection_executor(Connection&, const Executor&);

    native_conn_handle handle_;
    io_context* io_ = nullptr;
    stream_type socket_;
    oid_map_type oid_map_;
    Statistics statistics_;
    error_context error_context_;
};

/**
 * @ingroup group-connection-types
 * @brief Connection indicator
 *
 * This structure template should be specialized as `std::true_type`
 * for a type that models the `Connection` concept.
 */
template <typename, typename = std::void_t<>>
struct is_connection : std::false_type {};

template <typename ...Ts>
struct is_connection<connection<Ts...>> : std::true_type {};

/**
* @ingroup group-connection-concepts
* @brief Database connection concept
*
* `Connection` concept represents a minimum set of attributes and functions that are required
* by the library to establish communication and execute operations. `Connection` should provide:
* * the native PostgreSQL connection handle from `libpq`,
* * an executor to perform IO-related operation (according to the current version of Boost.Asio
*   it should be `boost::asio::io_context::executor_type` object),
* * an additional error context to provide context-depended information for errors,
* * IO functions that are necessary to perform operations.
*
* The default implementation of the concept is `ozo::connection`.
*
* ### Requirements
*
* Any wrapper object, which may be unwrapped to the underlying `Connection` model via
* `ozo::unwrap_connection` is valid `Connection` model.
*
* Connection `c` is an object of type `C` for which these next requirements are valid:
*
* | Expression | Type | Description |
* |------------|------|-------------|
* | <PRE>as_const(c).native_handle()</PRE> | `C::native_handle_type` | Should return native handle type of PostgreSQL connection. In the current implementation it should be `PGconn*` type. Shall not throw an exception. |
* | <PRE>c.oid_map()<sup>[1]</sup><br/>as_const(c).oid_map()<sup>[2]</sup></PRE> | `C::oid_map_type` | Should return a reference<sup>[1]</sup> or a const reference<sup>[2]</sup> on `OidMap` which is used by the connection, the library may update this map during connection establishing. Shall not throw an exception. |
* | <PRE>as_const(c).%get_error_context()</PRE> | `C::error_context_type` | Should return a const reference on an additional error context is related to at least the last error. In the current implementation, the type supported is `std::string`. Shall not throw an exception. |
* | <PRE>c.set_error_context(error_context)<sup>[1]</sup><br/>%c.set_error_context()<sup>[2]</sup></PRE> | | Should set<sup>[1]</sup> or reset<sup>[2]</sup> additional error context. |
* | <PRE>as_const(c).%get_executor()</PRE> | `C::executor_type` | Should provide an executor object that is useful for IO-related operations, like timer and so on. In the current implementation `boost::asio::io_context::executor_type` is only applicable. Shall not throw an exception. |
* | <PRE>c.set_executor(executor)</PRE> | `error_code` | Should change the executor for the specified one. This operation is used by `ozo::connection_pool` to provide connection migration between different instances of `boost::asio::io_service`. The call of the function during the active operation on connection is UB. The error should be indicated via the result. |
* | <PRE>c.assign(move(native_conn_handle))</PRE> | `error_code` | Should assign `ozo::native_conn_handle` to the connection object. If success - the previous handle should be destroyed. The error should be indicated via the result. |
* | <PRE>c.async_wait_write(WaitHandler)</PRE> | | Should asynchronously wait for write ready state of the connection socket. |
* | <PRE>c.async_wait_read(WaitHandler)</PRE> | | Should asynchronously wait for read ready state of the connection socket. |
* | <PRE>c.close()</PRE> | `error_code` | Should close connection socket and cancel all IO operation on the connection (like `async_wait_write`, `async_wait_read`). Shall not throw an exception. |
* | <PRE>%c.cancel()</PRE> | | Should cancel all IO operation on the connection (like `async_wait_write`, `async_wait_read`). Should not throw an exception. |
* | <PRE>ozo::get_connection(c, t, Handler)</PRE> | | Should reset the additional error context. This behaviour is performed in default implementation of `ozo::get_connection()` via `%c.set_error_context()` call. |
* | <PRE>ozo::is_connection<C></PRE> | `std::true_type` | The template `ozo::is_connection` should be specialized for the connection type via inheritance from `std::true_type`. |
*
* Where:
* * `Handler` is callable with `template <typename Connection> void(ozo::error_code, Connection&&)` signature,
* * `WaitHandler` is callable with `void(ozo::error_code, int = 0)` signature,
* * `t` is a `TimeConstraint` model object,
* * `as_const()` is `std::as_const()`,
* * `move()` is `std::move()`.
*
* @sa ozo::connection, ozo::get_connection(), ozo::is_connection
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
template <typename Connection>
inline auto get_executor(const Connection& conn) noexcept;

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
template <typename Connection>
inline std::string_view error_message(const Connection& conn);

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

template <typename T>
struct get_connection_type_default<T, Require<ozo::Connection<T>>> {
    using type = T;
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
