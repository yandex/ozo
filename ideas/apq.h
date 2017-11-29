using io_service = boost::asio::io_service;


/**
* Should we use defined type of pg_connect_handle, or some customized type and
* template for connection?
*   PGconn                       - pointer to native connection handle
*   std::function<void(PGconn*)> - finalization function, e.g. destructor for single 
*                                  connection or recycling operation for pool.
*/
using pg_connect_handle = std::shared_ptr<PGconn>; //, std::function<void(PGconn*)>>;


/**
* We can try to use standard error_code, but we will loose the error message
* from database. But if we will use the custom error_code we are forced to
* implement our own yield_context and async_result infrastructure. This perspective
* is not attractive. We can provide some kind of yield_context support, but with
* message loss in case of error_code bound to yield (i.e. yield[ec])
*/
using error_code = boost::system::error_code; // std:: / custom ?

/**
* Connection - represents binding of hative handle an io_service (io_context) object
* Should we allow to copy connection itself, or the object must be move-only?
*/
struct connection {
	io_service& ios;
	pg_connect_handle h;

	// Connection can be ConnectionProvider, so it provides itself
	template <typename Handler>
	friend void get_connection(connection c, Handler&& h) const {
		asio_handler_invoke([&]{ h(error_code{}, c);}, std::addressof(h));
	}
};

/**
* ConnectionProvider - concept provides applicable get_connection(Handler) function.
* Handler - concept of a callback with signature (error_code, connection). Connection
* is described below
*/

/**
* Function to get connection from provider. This customization point allows
* us to work with different kinds of connection providing. E.g. single
* connection, get connection from pool, lazy connection, retriable connection
* and so on.
*/
template <typename ConnectionProvider, typename CompletionToken>
auto get_connection(ConnectionProvider&& p, CompletionToken&& token) {
	detail::async_result_init<CompletionToken, void (error_code, connection)> init(std::move(token));
	p(std::forward<ConnectionProvider>(p), init.handler);
	return init.result.get();
}


struct dsn; // or just an alias to std::string?


class connection_pool {
public:
	template <typename Handler>
	void get_connection(io_service& ios, Handler&& h);
};

// Returns ConnectionProvider for specified dsn
auto make_connection_provider(io_service&, dsn); 

// Return ConnectionProvider for specified connection pool
auto make_connection_provider(io_service& ios, connection_pool& pool) {
	return [](auto&& handler) {
		pool.get_connection(ios, std::move(handler));
	}
}

enum class parameters_transfer_mode {binary, text};

template <typename T>
using cursor = std::unique_ptr<void(T&, void (error_code, size_type))>;

class transaction_connection;

/**
* Represents raw data row in case of request.
*/
class data_row;


template <parameters_transfer_mode prm>
struct basic_protocol {

	template <typename ConnectionProvider, typename Inserter, typename CompletionToken>
	static auto request(ConnectionProvider provider, query q, Inserter ins, CompletionToken token);

	/**
	* These two functions are dedicated to single row data fetching. Why two functions. We have experience
	* with single function design, and there are some Pros and Cons. In this case obtaining cursor and
	* fetching data are separated which removes a lot of problems of the cursor design which appear with
	* single function approach.
	*/
	template <typename ConnectionProvider, typename Query, typename T, typename CompletionToken>
	static auto request(ConnectionProvider provider, Query&& q, cursor<T>& c, CompletionToken token);

	template <typename T, typename CompletionToken>
	static auto fetch(cursor<T>& c, T& t, CompletionToken token);

	template <typename ConnectionProvider, typename Query, typename CompletionToken>
	static auto update(ConnectionProvider provider, Query&& q, CompletionToken token);

	template <typename ConnectionProvider, typename Query, typename CompletionToken>
	static auto execute(ConnectionProvider provider, Query q, CompletionToken token);

	template <typename ConnectionProvider, typename CompletionToken>
	static auto begin(ConnectionProvider provider, CompletionToken token); // Begin transaction

	template <typename CompletionToken>
	static auto begin(transaction_connection, CompletionToken) {
		static_assert(true, "can not begin transaction within transaction_connection");
	}

	template <typename ResultCompletionToken>
	static auto commit(transaction_connection conn, CompletionToken token) {
		return execute(conn, queries::commit, std::move(token));
	}

	template <typename ResultCompletionToken>
	static auto rollback(transaction_connection conn, CompletionToken token) {
		return execute(conn, queries::rollback, std::move(token));
	}
};

class transaction_connection {
	connection conn;
	std::unique_ptr<connection, void(connection*)> guard;
	friend struct basic_protocol;
public:
	transaction_connection() = default;

	template <typename Handler>
	friend void get_connection(const transaction& self, Handler&& h) {
		return get_connection(self.conn, std::forward<Handler>(h))
	}
};

using binary_protocol = basic_protocol<parameters_transfer_mode::binary>;

using text_protocol = basic_protocol<parameters_transfer_mode::text>;


template <parameters_transfer_mode prm>
template <typename ConnectionProvider, typename Query, typename Inserter, typename CompletionToken>
inline auto basic_protocol<prm>::request(ConnectionProvider provider, Query&& q, Inserter ins, CompletionToken token) {
	detail::async_result_init<CompletionToken, void (error_code, size_type)> init(std::move(token));

	get_connection(provider, [q, ins, h = init.handler] (error_code ec, connection conn) {
		if (ec) {
			return asio_handler_invoke([ec, h]{ h(ec);}, std::addressof(h));
		}
		auto op = detail::make_request_op(conn, q, ins, h);
		op.perform();
	});

	return init.result.get();
}

template <parameters_transfer_mode prm>
template <typename ConnectionProvider, typename Query, typename T, typename CompletionToken>
inline auto basic_protocol<prm>::request(ConnectionProvider provider, Query&& q, cursor<T>& c, CompletionToken token) {
	detail::async_result_init<CompletionToken, void (error_code)> init(std::move(token));

	get_connection(provider, [q, &c, h = init.handler] (error_code ec, connection conn) {
		if (ec) {
			return asio_handler_invoke([ec, h]{ h(ec);}, std::addressof(h));
		}
		auto op = detail::make_request_cursor_op(conn, q, c, h);
		op.perform();
	});

	return init.result.get();
}

template <parameters_transfer_mode prm>
template <typename T, typename CompletionToken>
inline auto basic_protocol<prm>::fetch(cursor<T>& c, T& t, CompletionToken token) {
	detail::async_result_init<CompletionToken, void (error_code, size_type)> init(std::move(token));

	if (!c) {
		asio_handler_invoke([h = init.handler]{ h(error_code{}, 0);}, std::addressof(h));
	}
	auto op = detail::make_fetch_op(c, init.handler);
	op.perform();

	return init.result.get();
}

template <parameters_transfer_mode prm>
template <typename ConnectionProvider, typename Query, typename CompletionToken>
inline auto basic_protocol<prm>::update(ConnectionProvider provider, Query&& q, CompletionToken token) {
	detail::async_result_init<CompletionToken, void (error_code, size_type)> init(std::move(token));

	get_connection(provider, [q, h = init.handler] (error_code ec, connection conn) {
		if (ec) {
			return asio_handler_invoke([ec, h]{ h(ec);}, std::addressof(h));
		}
		auto op = detail::make_update_op(conn, q, h);
		op.perform();
	});

	return init.result.get();
}

template <parameters_transfer_mode prm>
template <typename ConnectionProvider, typename Query, typename CompletionToken>
inline auto basic_protocol<prm>::execute(ConnectionProvider provider, Query q, CompletionToken token) {
	detail::async_result_init<CompletionToken, void (error_code)> init(std::move(token));

	get_connection(provider, [q, h = init.handler] (error_code ec, connection conn) {
		if (ec) {
			return asio_handler_invoke([ec, h]{ h(ec);}, std::addressof(h));
		}
		auto op = detail::make_execute_op(conn, q, h);
		op.perform();
	});

	return init.result.get();
}

template <parameters_transfer_mode prm>
template <typename ConnectionProvider, typename CompletionToken>
inline auto basic_protocol<prm>::begin(ConnectionProvider provider, CompletionToken token) {

	const auto fwd = [](auto h, error_code ec, transaction_connection c) {
		asio_handler_invoke([&]{ h(ec, c);}, std::addressof(h)); // io.post()?
	};

	detail::async_result_init<CompletionToken, void (error_code)> init(std::move(token));
	
	get_connection(provider, [fwd, token = init.handler](error_code ec, connection conn) {
		if (ec) {
			return fwd(std::move(token), ec, {});
		}
		basic_protocol::execute(conn, queries::begin_transaction, 
				[fwd, conn, token = std::move(token)] (error_code ec) {
			if (ec) {
				return fwd(std::move(token), ec, {});
			}
			fwd(std::move(token), {}, {conn});
		});
	});

	return init.result.get();
}; // Begin transaction


make_query( "SELECT name FROM my_table WHERE id="_sql << id << " AND sid=" << sid );

make_query( "SELECT name FROM my_table WHERE id=:id  AND sid=:sid"_sql % (("id",id), ("sid",sid)) );


BOOST_FUSION_DEFINE_STRUCT((), (my_query_params), 
	(std::string, id)
	(int, sid)
);

/**
* Query from text and params
*/
using my_query = query<"SELECT name FROM my_table WHERE id=:id  AND sid=:sid"_sql, my_query_params>;

/**
* Query for reading from query.conf
*/
using my_query = query<"my_query"_query, my_query_params>;

enum class debug_mode {off, on};

/******************************************************
* Minimal query interface definition.
*******************************************************/

/**
* Function returns text part of the query
*/
template <typename Query>
inline auto query_text(const Query& q) {
    return q.text();
}

/**
* Returns an adapted sequence (or tuple) with values
*/
template <typename Query>
inline auto query_values(const Query& q) {
    return q.values();
}

/**
* returns query execution time limitation
*/
template <typename Query>
inline std::chrono::steady_clock::duration query_timeout(const Query& q) {
    return q.timeout();
}

/**
* Returns query execute strategy - the object to determine execution and
* retries. May be added later. But this must be one of the key features.
*/
template <typename Query>
inline auto query_execute_strategy(const Query& q) {
    return q.strategy();
}


/***********************************************
* Type mapping traits
***********************************************/
template <typename T>
struct type_traits;

using oid_t = int64_t; // ????

template <typename T>
constexpr auto get_type_traits(const T&) noexcept {
    return type_traits<std::decay_t<T>>{};
}

template <typename T>
constexpr auto type_name(const T& v) noexcept {
    return get_type_traits(v)::name();
}

template <typename T>
constexpr std::size_t type_size(const T& v) noexcept {
    return get_type_traits(v)::size;
}

// Overload trait for std::string for example
template <>
inline std::size_t type_size(const std::string& v) noexcept {
    return v.size();
}

/**
* Oid acquisition for types must looks like (simplified):
* auto oid = type_oid(v, [oid_map](const auto& v) {
*         return oid_map.at(type_name(v));
*     });
*/

// Oid acquisition for built-in PostgreSQL types
template <typename T>
constexpr std::enable_if_t<is_built_in_v<T>, oid_t> type_oid(const T& v, ...) noexcept {
    return get_type_traits(v)::oid;
}

// Oid acquisition for custom types
template <typename T, typename OidProvider>
inline std::enable_if_t<!is_built_in_v<T>, oid_t> type_oid(const T& v, OidProvider get) {
    return get(v);
}
