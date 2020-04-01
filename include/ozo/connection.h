#pragma once

#include <ozo/error.h>
#include <ozo/type_traits.h>
#include <ozo/asio.h>
#include <ozo/core/concept.h>
#include <ozo/core/recursive.h>
#include <ozo/core/none.h>
#include <ozo/deadline.h>
#include <ozo/pg/handle.h>

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

using no_statistics = ozo::none_t;

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
 * E.g. such overload is used for the `ozo::pooled_connection` of this library.
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
 * @param conn --- wrapped or unwrapped `Connection`
 * @return unwrapped `Connection` object reference
*/
template <typename T>
inline constexpr decltype(auto) unwrap_connection(T&& conn) noexcept {
    return detail::apply<unwrap_connection_impl>(std::forward<T>(conn));
}

/**
 * @brief Default model for `Connection` concept
 *
 * `Connection` concept model which is used by the library as default model.
 * The class object is non-copyable.
 *
 * @tparam OidMap --- oid map of types are used with connection
 * @tparam Statistics --- statistics of the connection (not supported yet)
 *
 * @thread_safety{Safe,Unsafe}
 * @ingroup group-connection-types
 * @models{Connection}
 */
template <typename OidMap, typename Statistics>
class connection {
public:
    using native_handle_type = ozo::pg::conn::pointer; //!< Native connection handle type
    using oid_map_type = OidMap; //!< Oid map of types that are used with the connection
    using error_context_type = std::string; //!< Additional error context which could provide context depended information for errors
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
     * @warning Don't use the function with the `PQfinish` function. Use the `close()` member
     *          function instead.
     *
     * @return native_handle_type --- native connection handle.
     */
    native_handle_type native_handle() const noexcept { return handle_.get(); }

    /**
     * Get a reference to an oid map object for types that are used with the connection.
     * This method is used after connection establishing process to update the map.
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

    template <typename Key, typename Value>
    void update_statistics(const Key&, const Value&) noexcept {
        static_assert(std::is_void_v<Key>, "update_statistics is not supperted");
    }
    const Statistics& statistics() const noexcept { return statistics_;}

    /**
     * Get the additional context object for an error that occurred during the last operation on the connection.
     *
     * @return const error_context_type& --- additional context for the error
     */
    const error_context_type& get_error_context() const noexcept { return error_context_; }
    /**
     * Set the additional error context object. This function may be used to provide additional context-depended
     * data that is related to the current operation error.
     *
     * @param v --- new error context.
     */
    void set_error_context(error_context_type v = error_context_type{}) { error_context_ = std::move(v); }

    /**
     * Get the executor associated with the object.
     *
     * @return executor_type --- executor object.
     */
    executor_type get_executor() const noexcept { return io_->get_executor(); }

    /**
     * Assign an existing native connection handle to the object.
     *
     * Typically this function is used by the library within the connection establishing process.
     * Users should not use it directly other than for a special purpose (e.g., special connection pool
     * implementation and so on).
     *
     * @warning Don't call this function during any active operation executed on the object.
     *
     * @param handle --- rvalue reference on a new handle.
     * @return error_code --- error code of the function call.
     */
    error_code assign(ozo::pg::conn&& handle);

    /**
     * Release ownership of the native connection handle object.
     *
     * This function may be used to obtain the underlying representation of the descriptor.
     * After calling this function, is_open() returns false. The caller is the owner for
     * the connection descriptor. All outstanding asynchronous read or write operations will
     * finish immediately, and the handlers for cancelled operations will be passed the
     * `boost::asio::error::operation_aborted` error.
     *
     * @return ozo::pg::conn --- native connection handle object
     */
    ozo::pg::conn release();

    /**
     * Asynchronously wait for the connection socket to become ready to write or to have pending error conditions.
     *
     * Typically this function is used by the library within the connection establishing process and operation execution.
     * Users should not use it directly other than for custom `libpq`-based opeartions.
     *
     * @warning The function is designated to the library operations use only. Don't call this function directly.
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
     * @warning The function is designated to the library operations use only. Don't call this function directly.
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

    ~connection();
private:
    using stream_type = asio::posix::stream_descriptor;

    ozo::pg::conn handle_;
    io_context* io_ = nullptr;
    stream_type socket_;
    oid_map_type oid_map_;
    Statistics statistics_;
    error_context_type error_context_;
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
* `%Connection` concept represents a minimum set of attributes and functions that are required
* by the library to execute operations on a database. `%Connection` should provide:
* * the native PostgreSQL connection handle from `libpq`,
* * an executor to perform IO-related operation (according to the current version of Boost.Asio
*   it should be `boost::asio::io_context::executor_type` object),
* * an additional error context to provide context-depended information for errors,
* * IO functions that are necessary to perform operations.
*
* @par Concrete models
*
* `ozo::connection`, `ozo::pooled_connection`, `ozo::transaction`
*
* @par Definition
*
* Any wrapper object, which may be unwrapped to the underlying `%Connection` model via
* `ozo::unwrap_connection` is valid `%Connection` model.
*
* %Connection `c` is an object of type `C` for which these next requirements are valid:
*
* | Expression | Type | Description |
* |------------|------|-------------|
* | <PRE>as_const(c).native_handle()</PRE> | `C::native_handle_type` | Should return native handle type of PostgreSQL connection. In the current implementation it should be `PGconn*` type. Shall not throw an exception. |
* | <PRE>as_const(c).oid_map()</PRE> | `C::oid_map_type` | Should return a const reference on `OidMap` which is used by the library for custom types introspection for the connection IO. Shall not throw an exception. |
* | <PRE>as_const(c).%get_error_context()</PRE> | `C::error_context_type` | Should return a const reference on an additional error context is related to at least the last error. In the current implementation, the type supported is `std::string`. Shall not throw an exception. |
* | <PRE>c.set_error_context(error_context_type)<sup>[1]</sup><br/>%c.set_error_context()<sup>[2]</sup></PRE> | | Should set<sup>[1]</sup> or reset<sup>[2]</sup> additional error context. |
* | <PRE>as_const(c).%get_executor()</PRE> | `C::executor_type` | Should provide an executor object that is useful for IO-related operations, like timer and so on. In the current implementation `boost::asio::io_context::executor_type` is only applicable. Shall not throw an exception. |
* | <PRE>c.async_wait_write(WaitHandler)</PRE> | | Should asynchronously wait for write ready state of the connection socket. |
* | <PRE>c.async_wait_read(WaitHandler)</PRE> | | Should asynchronously wait for read ready state of the connection socket. |
* | <PRE>c.close()</PRE> | `error_code` | Should close connection socket and cancel all IO operation on the connection (like `async_wait_write`, `async_wait_read`). Shall not throw an exception. |
* | <PRE>%c.cancel()</PRE> | | Should cancel all IO operation on the connection (like `async_wait_write`, `async_wait_read`). Should not throw an exception. |
* | <PRE>%c.is_bad()</PRE> | bool | Should return `false` for the established connection that can perform operations. Shall not throw an exception. |
* | <PRE>%c.is_open()</PRE> | bool | Should return `true` for the object with valid native connection handle attached. Shall not throw an exception. |
* | <PRE>%bool(as_const(c))</PRE> | bool | Should return `true` for the established connection that can perform operations. Should return `false` at least if `c.is_bad()` returns `true`. Shall not throw an exception. |
* | <PRE>ozo::get_connection(c, t, Handler)</PRE> | | Should reset the additional error context. This behaviour is performed in default implementation of `ozo::get_connection()` via `%c.set_error_context()` call. |
* | <PRE>ozo::is_connection<C></PRE> | `std::true_type` | The template `ozo::is_connection` should be specialized for the connection type via inheritance from `std::true_type`. |
*
* Where:
* * `Handler` is callable with `template <typename Connection> void(ozo::error_code, Connection&&)` signature,
* * `WaitHandler` is callable with `void(ozo::error_code, int = 0)` signature,
* * `t` is a `TimeConstraint` model object,
* * `as_const()` is `std::as_const()`.
*
* @sa ozo::get_connection(), ozo::is_connection
* @concept{Connection}
*/
//! @cond
template <typename T>
inline constexpr auto Connection = is_connection<std::decay_t<decltype(unwrap_connection(std::declval<T>()))>>::value;
//! @endcond

/**
 * @defgroup group-connection-functions Related functions
 * @ingroup group-connection
 * @brief Connection related functions
 */
///@{

/**
 * @brief Get native connection handle object.
 *
 * Alias to `unwrap_connection(conn).native_handle()`. See the `Connection` documentation for more details.
 *
 * @param conn --- `Connection` object.
 * @return native connection handle.
 */
template <typename Connection>
inline auto get_native_handle(const Connection& conn) noexcept;

/**
 * @brief Get the executor associated with the object.
 *
 * Alias to `unwrap_connection(conn).get_executor()`. See the `Connection` documentation for more details.
 *
 * @param conn --- `Connection` object.
 * @return executor associated with the object.
 */
template <typename Connection>
inline auto get_executor(const Connection& conn) noexcept;

/**
 * @brief Determine whether the connection is in bad state.
 *
 * Alias to `unwrap_connection(conn).is_bad()`. See the `Connection` documentation for more details.
 *
 * @param conn --- `Connection` object.
 * @return `true` if connection is in bad or null state, `false` - otherwise.
 */
template <typename T>
inline bool connection_bad(const T& conn) noexcept;

/**
 * @brief Indicates if connection state is not bad.
 *
 * Alias to `!ozo::connection_bad(conn)`.
 *
 * @param conn --- `Connection` object.
 * @return `false` if connection is in bad state, `true` - otherwise
 */
template <typename Connection>
inline bool connection_good(const Connection& conn) noexcept {
    return !connection_bad(conn);
}

/**
 * @brief Get native libpq error message
 *
 * Underlying libpq provides additional textual context for different errors which
 * can be while interacting via connection. This function gives access for such messages.
 *
 * @param conn --- `Connection` to get message from.
 * @return `std::string_view` with a message.
 */
template <typename Connection>
inline std::string_view error_message(const Connection& conn);

/**
 * @brief Get additional error context.
 *
 * Alias to `unwrap_connection(conn).get_error_context()`. See. `Connection` documentation for more details.
 *
 * @param conn --- `Connection` object which is not in null recursive state
 * @return reference on additional context
 */
template <typename Connection>
inline const auto& get_error_context(const Connection& conn);

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
 * @brief %Connection type getter
 *
 * This type describes the connection type from `ConnectionProvider` or `ConnectionSource`.
 *
 * @tparam ConnectionProvider - connection provider type to get `Connection` type from.
 *
 * By default it assumes on `ConnectionProvider::connection_type` type, if no nested type
 * `connection_type` is found no inner type `type`.
 * The implementation example (exposition only):
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
 * Here you can specify how to obtain `Connection` type from the custom `ConnectionProvider`.
 * E.g. for custom `Connection` implementation which is used via pointer the specialization
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
    using type = <implementation defined>; ///!< Type of the `Connection` object provided by the `ConnectionProvider` specified
};
#else
 : detail::get_connection_type_default<ConnectionProvider>{};
#endif

/**
 * @brief The connection object type that `ConnectionProvider` or `ConnectionSource` provide
 *
 * This type alias can be used to determine exact type of a connection object which can be obtained from a
 * `ConnectionSource` or `ConnectionProvider`. It uses `ozo::get_connection_type` metafunction
 * to get a `Connection` type.
 *
 * @tparam T - `ConnectionSource` or `ConnectionProvider` type.
 * @ingroup group-connection-types
 */
template <typename T>
using connection_type = typename get_connection_type<std::decay_t<T>>::type;

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
        unwrap_connection(c).set_error_context();
        auto ex = unwrap_connection(c).get_executor();
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
 * @brief %Connection source concept
 *
 * `%ConnectionSource` concept represents a type that creates a connection object
 * with an established connection to a database. The connection source is the IO
 * execution context agnostic entity. `%ConnectionSource` is used by the
 * `ozo::connection_provider` or other `%ConnectionSource` models like `ozo::connection_pool`.
 * Typically there is no need to use it directly.
 *
 * @par Concrete models
 *
 * `ozo::connection_info`, `ozo::connection_pool`, `ozo::failover::role_based_connection_source`.
 *
 * @par Definition
 *
 * %Connection source `s` is an object of type `S` for which these next requirements
 * are valid:
 *
 * | Expression | Type | Description |
 * |------------|------|-------------|
 * | <PRE>s(io, t, h)</PRE> | | Should invoke a continuation object `h` within time-limited by `t` (if not `ozo::none` specified) with a connection object associated with an executor object of the `io` execution context or/and `error_code` in case of failure. If there is no error_code code set, the connection object should be in a valid state. |
 * | <PRE>ozo::connection_type<S></PRE> | deduced type | Should return the `Connection` object type that is produced by the source. |
 * | <PRE>ozo::Connection<ozo::connection_type<S>></PRE> | `std::true_type` | Should provide a connection type that models the `Connection` concept. |
 *
 * Where:
 * * `io` execution context object with type `ozo::io_context`,
 * * `t` is the `TimeConstraint` model object,
 * * `h` is callable with `void(ozo::error_code, ozo::connection_type<S>)` signature.
 *
 * @note The connection source should be an IO execution context agnostic entity.
 *
 * @ingroup group-connection-concepts
 * @concept{ConnectionSource}
 */
//! @cond
template <typename T>
inline constexpr auto ConnectionSource = is_connection_source<std::decay_t<T>>::value;
//! @endcond

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
 * @brief %Connection provider concept
 *
 * `%ConnectionProvider` concept represents a type that creates a connection object with an
 * established connection to a database. The connection provider is bound to the IO execution
 * context object. `%ConnectionProvider` is used by the library operations to get a connection
 * object.
 *
 * Typically it is enough to use the `ConnectionSource` model with `ozo::connection_provider`,
 * but sometimes it is more convenient to model the `%ConnectionProvider` with type, e.g. a
 * user has a special connection pool with deferred `io_context` binding algorithm. So, in
 * this case, this type should conform to the concept definition.
 *
 * @par Concrete models
 *
 * `ozo::connection_provider`, `Connection` model is a `%ConnectionProvider` by default.
 *
 * @par Definition
 *
 * %Connection provider `p` is an object of type `P` for which these next requirements
 * are valid:
 *
 * | Expression | Type | Description |
 * |------------|------|-------------|
 * | <PRE>ozo::get_connection(p, t, h)</PRE> | | Should invoke a continuation object `h` within time-limited by `t` (if not `ozo::none` specified) with a connection object associated with an executor object or/and `error_code` in case of failure. If no `error_code` code is set the connection object should be in a valid state. |
 * | <PRE>ozo::connection_type<P></PRE> | deduced type | Should return a type of the `Connection` object that is produced by the provider. |
 * | <PRE>ozo::Connection<ozo::connection_type<P>></PRE> | `std::true_type` | Should provide a connection type that models the `Connection` concept. |
 *
 * Where:
 * * `t` is the `TimeConstraint` model object,
 * * `h` is callable with `void(ozo::error_code, ozo::connection_type<S>)` signature.
 *
 * @ingroup group-connection-concepts
 * @concept{ConnectionProvider}
 */
//! @cond
template <typename T>
inline constexpr auto ConnectionProvider = is_connection_provider<std::decay_t<T>>::value;
//! @endcond

#ifdef OZO_DOCUMENTATION
/**
 * @brief Get a connection object from connection provider with time constaint.
 *
 * Retrives a connection object from connection provider within specified time constaint.
 * The default implementation for the `Connection` object forwards this object and resets
 * it's error context.
 *
 * @note The function does not particitate in ADL since could be implemented via functional object.
 *
 * @param provider --- `ConnectionProvider` to get connection from.
 * @param time_constraint --- #TimeConstraint for the operation.
 * @param token --- operation #CompletionToken.
 * @return deduced from #CompletionToken.
 *
 * @par Customization point
 *
 * This is a customization point of `ConnectionProvider`. By default `ConnectionProvider` should have
 * `async_get_connection()` member function with signature:
 * @code
    template <typename TimeConstraint>
    void async_get_connection(TimeConstraint t, Handler&& h);
 * @endcode
 *
 * This behaviour can be customized via `async_get_connection_impl` specialization. E.g. for custom implementation
 * of `Connection` customization may looks like this (*exposition only*):
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
 * @brief Get a connection from connection provider
 *
 * This function is a shortcut to `ozo::get_connection(provider, ozo::none, token)` call.
 *
 * @note The function does not particitate in ADL since could be implemented via functional object.
 *
 * @param provider --- `ConnectionProvider` to get connection from.
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

inline constexpr get_connection_op<detail::initiate_async_get_connection> get_connection;
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
 * @param conn --- `Connection` to be closed
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
 * @param conn --- pointer on a `Connection` to be closed.
 *
 * @par Example
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

} // namespace ozo

#include <ozo/impl/connection.h>
