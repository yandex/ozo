#pragma once
#include <iostream>

#include <apq/async_result.h>
#include <apq/error.h>
#include <apq/type_traits.h>
#include <apq/asio.h>
#include <apq/concept.h>

#include <apq/detail/bind.h>
#include <apq/impl/connection.h>

namespace libapq {

using impl::pg_conn_handle;

using no_statistics = decltype(hana::make_map());

/**
* Marker to tag a connection type.
*/
template <typename, typename = std::void_t<>>
struct is_connection : std::false_type {};

/**
* We define the connection to database not as a concrete class or type,
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
*     Must return reference or proxy for pg_conn_handle object
*
*   * get_connection_error_context()
*     Must return reference or proxy for additional error context. There is no
*     mechanism to privide context depent information via [std|boost]::error_code. So
*     we decide to use connection for this purpose since native libpq error message is
*     bound to connection and all asyncronous operation of the library is bound to
*     connection too. Currently std::string supported as a context object.
*/
template <typename T>
struct is_connection<T, std::void_t<
    decltype(get_connection_oid_map(std::declval<T&>())),
    decltype(get_connection_socket(std::declval<T&>())),
    decltype(get_connection_handle(std::declval<T&>())),
    decltype(get_connection_error_context(std::declval<T&>())),
    decltype(get_connection_oid_map(std::declval<const T&>())),
    decltype(get_connection_socket(std::declval<const T&>())),
    decltype(get_connection_handle(std::declval<const T&>())),
    decltype(get_connection_error_context(std::declval<const T&>()))
>> : std::true_type {};


template <typename, typename = std::void_t<>>
struct is_connection_wrapper : std::false_type {};

template <typename T>
struct is_connection_wrapper<::std::shared_ptr<T>> : std::true_type {};
template <typename T, typename D>
struct is_connection_wrapper<::std::unique_ptr<T, D>> : std::true_type {};
template <typename T>
struct is_connection_wrapper<::std::optional<T>> : std::true_type {};
template <typename T>
struct is_connection_wrapper<::boost::shared_ptr<T>> : std::true_type {};
template <typename T>
struct is_connection_wrapper<::boost::scoped_ptr<T>> : std::true_type {};
template <typename T>
struct is_connection_wrapper<::boost::optional<T>> : std::true_type {};

template <typename T>
constexpr auto ConnectionWrapper = is_connection_wrapper<std::decay_t<T>>::value;

template <typename T>
constexpr auto Connection = is_connection<std::decay_t<T>>::value;

/**
* Connection type traits
*/
template <typename T>
struct connection_traits {
    using oid_map = std::decay_t<decltype(get_connection_oid_map(std::declval<T&>()))>;
    using socket = std::decay_t<decltype(get_connection_socket(std::declval<T&>()))>;
    using handle = std::decay_t<decltype(get_connection_handle(std::declval<T&>()))>;
    using error_context = std::decay_t<decltype(get_connection_error_context(std::declval<T&>()))>;
};

/**
* Connection public access functions section 
*/

/**
* Overloaded version of unwrap function for Connection.
* Returns connection object itself.
*/
template <typename T>
inline decltype(auto) unwrap_connection(T&& conn, 
        Require<!ConnectionWrapper<T>>* = 0) noexcept {
    return std::forward<T>(conn);
}

/**
* Unwraps wrapped connection recusively.
* Returns unwrapped connection object.
*/
template <typename T>
inline decltype(auto) unwrap_connection(T&& conn, 
        Require<ConnectionWrapper<T>>* = 0) noexcept {
    return unwrap_connection(*conn);
}

template <typename, typename = std::void_t<>>
struct is_connectiable : std::false_type {};

template <typename T>
struct is_connectiable <T, std::void_t<
    decltype(unwrap_connection(std::declval<T&>()))
>>: std::integral_constant<bool, Connection<
    decltype(unwrap_connection(std::declval<T&>()))
>> {};

template <typename T>
constexpr auto Connectable = is_connectiable<std::decay_t<T>>::value;

/**
* Function to get connection traits type from object
*/
template <typename T, typename = Require<Connectable<T>>>
constexpr connection_traits<std::decay_t<decltype(unwrap_connection(std::declval<T>()))>>
    get_connection_traits(T&&) noexcept {return {};}

/**
* Returns PostgreSQL connection handle of the connection.
*/
template <typename T, typename = Require<Connectable<T>>>
inline decltype(auto) get_handle(T&& conn) noexcept {
    using impl::get_connection_handle;
    return get_connection_handle(
        unwrap_connection(std::forward<T>(conn)));
}

/**
* Returns PostgreSQL native connection handle of the connection.
*/
template <typename T, typename = Require<Connectable<T>>>
inline decltype(auto) get_native_handle(T&& conn) noexcept {
    return get_handle(std::forward<T>(conn)).get();
}

/**
* Returns socket stream object of the connection.
*/
template <typename T, typename = Require<Connectable<T>>>
inline decltype(auto) get_socket(T&& conn) noexcept {
    using impl::get_connection_socket;
    return get_connection_socket(unwrap_connection(std::forward<T>(conn)));
}

/**
* Returns io_context for connection is bound to.
*/
template <typename T, typename = Require<Connectable<T>>>
inline decltype(auto) get_io_context(T& conn) noexcept {
    return get_socket(conn).get_io_service();
}

/**
* Rebinds io_context for the connection
*/
template <typename T, typename = Require<Connectable<T>>>
inline error_code rebind_io_context(T& conn, io_context& io) {
    using impl::rebind_connection_io_context;
    return rebind_connection_io_context(unwrap_connection(conn), io);
}

/**
* Indicates if connection is bad
*/
template <typename T>
inline Require<Connectable<T> && !OperatorNot<T>,
bool> connection_bad(const T& conn) noexcept { 
    using impl::connection_status_bad;
    return connection_status_bad(get_native_handle(conn));
}

/**
* Indicates if connection is bad. Overloaded version for
* ConnectionWraper with operator !
*/
template <typename T>
inline Require<ConnectionWrapper<T> && OperatorNot<T>,
bool> connection_bad(const T& conn) noexcept {
    using impl::connection_status_bad;
    return !conn || connection_status_bad(get_native_handle(conn));
}

/**
* Indicates if connection is not bad
*/
template <typename T, typename = Require<Connectable<T>>>
inline bool connection_good(const T& conn) noexcept {
    return !connection_bad(conn);
}

/**
* Returns string view on native libpq connection error
*/
template <typename T, typename = Require<Connectable<T>>>
inline std::string_view error_message(T&& conn) {
    using impl::connection_error_message;
    return connection_error_message(get_native_handle(conn));
}

/**
* Getter of additional error context. for error occured while operating
* with connection.
*/
template <typename T, typename = Require<Connectable<T>>>
inline const auto& get_error_context(const T& conn) {
    using impl::get_connection_error_context;
    return get_connection_error_context(unwrap_connection(conn));
}

/**
* Set the connection error context.
*/
template <typename T, typename Ctx>
inline Require<Connectable<T>> set_error_context(T& conn, Ctx&& ctx) {
    using impl::get_connection_error_context;
    get_connection_error_context(unwrap_connection(conn)) = std::forward<Ctx>(ctx);
}

/**
* Reset the connection error context.
*/
template <typename T>
inline Require<Connectable<T>> reset_error_context(T& conn) {
    using ctx_type = std::decay_t<decltype(get_error_context(conn))>;
    set_error_context(conn, ctx_type{});
}

/**
* Returns type oids map of the connection
*/
template <typename T, typename = Require<Connectable<T>>>
inline decltype(auto) get_oid_map(T&& conn) noexcept {
    using impl::get_connection_oid_map;
    return get_connection_oid_map(unwrap_connection(std::forward<T>(conn)));
}

/**
* Returns statistics object of the connection
*/
template <typename T, typename = Require<Connectable<T>>>
inline decltype(auto) get_statistics(T&& conn) noexcept {
    using impl::get_connection_statistics;
    return get_connection_statistics(unwrap_connection(std::forward<T>(conn)));
}

// Oid Map helpers

template <typename T, typename C, typename = Require<Connectable<C>>>
inline decltype(auto) type_oid(C&& conn) noexcept {
    return type_oid<std::decay_t<T>>(
        get_oid_map(std::forward<C>(conn)));
}

template <typename T, typename C, typename = Require<Connectable<C>>>
inline decltype(auto) type_oid(C&& conn, const T&) noexcept {
    return type_oid<std::decay_t<T>>(std::forward<C>(conn));
}

template <typename T, typename C, typename = Require<Connectable<C>>>
inline void set_type_oid(C&& conn, oid_t oid) noexcept {
    set_type_oid<T>(get_oid_map(std::forward<C>(conn)), oid);
}


template <typename ConnectionProvider, typename Enable = void>
struct get_connectiable_type {
    using type = typename std::decay_t<ConnectionProvider>::connectiable_type;
};

template <typename ConnectionProvider>
using connectiable_type = typename get_connectiable_type<ConnectionProvider>::type;

/**
* Connection is Connection Provider itself, so we need to define connection
* type it provides.
*/
template <typename T>
struct get_connectiable_type<T, Require<Connectable<T>>> {
    using type = std::decay_t<T>;
};

template <typename, typename = std::void_t<>>
struct is_connection_provider_functor : std::false_type {};
template <typename T>
struct is_connection_provider_functor<T, std::void_t<
    decltype( std::declval<T>() (std::declval<void(error_code, connectiable_type<T>)>()) )
>> : std::true_type {};

template <typename T>
constexpr auto ConnectionProviderFunctor = is_connection_provider_functor<std::decay_t<T>>::value;

/**
* Function to get connection from provider asynchronously via callback. 
* This is customization point which allows to work with different kinds
* of connection providing. E.g. single connection, get connection from 
* pool, lazy connection, retriable connection and so on. 
*/
template <typename T, typename Handler>
inline Require<ConnectionProviderFunctor<T>>
async_get_connection(T&& provider, Handler&& handler) {
    provider(std::forward<Handler>(handler));
}

template <typename T, typename Handler>
inline Require<Connectable<T>>
async_get_connection(T&& conn, Handler&& handler) {
    reset_error_context(conn);
    decltype(auto) io = get_io_context(conn);
    io.dispatch(detail::bind(
        std::forward<Handler>(handler), error_code{}, std::forward<T>(conn)));
}

template <typename, typename = std::void_t<>>
struct is_connection_provider : std::false_type {};
template <typename T>
struct is_connection_provider<T, std::void_t<decltype(
    async_get_connection(
        std::declval<T&>(),
        std::declval<std::function<void(error_code, connectiable_type<T>)>>()
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
template <typename T, typename CompletionToken, typename = Require<ConnectionProvider<T>>>
auto get_connection(T&& provider, CompletionToken&& token) {
    using signature_t = void (error_code, connectiable_type<T>);
    async_completion<std::decay_t<CompletionToken>, signature_t> init(token);

    async_get_connection(std::forward<T>(provider), 
            std::move(init.completion_handler));

    return init.result.get();
}

} // namespace libapq
