#pragma once

#include <ozo/detail/cancel_timer_handler.h>
#include <ozo/detail/post_handler.h>
#include <ozo/detail/timeout_handler.h>
#include <ozo/impl/io.h>
#include <ozo/io/binary_query.h>
#include <ozo/connection.h>
#include <ozo/query_builder.h>
#include <ozo/deadline.h>

#include <boost/asio/bind_executor.hpp>
#include <boost/asio/coroutine.hpp>

namespace ozo {
namespace impl {

template <typename Connection, typename Handler>
struct request_operation_context {
    std::decay_t<Connection> conn;
    std::decay_t<Handler> handler;
    query_state state = query_state::send_in_progress;

    using result_type = std::decay_t<decltype(get_result(conn))>;
    result_type result;

    request_operation_context(Connection conn, Handler handler)
      : conn(std::forward<Connection>(conn)),
        handler(std::forward<Handler>(handler)) {}
};

template <typename Connection, typename Handler>
inline decltype(auto) make_request_operation_context(Connection&& conn, Handler&& h) {
    auto allocator = asio::get_associated_allocator(h);
    return std::allocate_shared<request_operation_context<Connection, Handler>>(
        allocator, std::forward<Connection>(conn), std::forward<Handler>(h)
    );
}

template <typename ...Ts>
using request_operation_context_ptr = std::shared_ptr<request_operation_context<Ts...>>;

template <typename ...Ts>
inline auto& get_connection(const request_operation_context_ptr<Ts...>& ctx) noexcept {
    return ctx->conn;
}

template <typename ...Ts>
inline query_state get_query_state(const request_operation_context_ptr<Ts...>& ctx) noexcept {
    return ctx->state;
}

template <typename ...Ts>
inline void set_query_state(const request_operation_context_ptr<Ts...>& ctx,
        query_state state) noexcept {
    ctx->state = state;
}

template <typename ...Ts>
inline auto& get_request_result(const request_operation_context_ptr<Ts...>& ctx) noexcept {
    return ctx->result;
}

template <typename ...Ts>
inline void set_request_result(const request_operation_context_ptr<Ts...>& ctx,
        typename request_operation_context<Ts...>::result_type value) noexcept {
    ctx->result = std::move(value);
}

template <typename ... Ts>
auto& get_handler(const request_operation_context_ptr<Ts ...>& context) noexcept {
    return context->handler;
}

template <typename ...Ts>
inline void done(const request_operation_context_ptr<Ts...>& ctx, error_code ec) {
    set_query_state(ctx, query_state::error);
    decltype(auto) conn = get_connection(ctx);
    error_code _;
    get_socket(conn).cancel(_);
    std::move(get_handler(ctx))(std::move(ec), conn);
}

template <typename ...Ts>
inline void done(const request_operation_context_ptr<Ts...>& ctx) {
    std::move(get_handler(ctx))(error_code {}, get_connection(ctx));
}

template <typename Continuation, typename ...Ts>
inline void write_poll(const request_operation_context_ptr<Ts...>& ctx, Continuation&& c) {
    write_poll(get_connection(ctx), std::forward<Continuation>(c));
}

template <typename Continuation, typename ...Ts>
inline void read_poll(const request_operation_context_ptr<Ts...>& ctx, Continuation&& c) {
    read_poll(get_connection(ctx), std::forward<Continuation>(c));
}

template <typename Context, typename BinaryQuery>
struct async_send_query_params_op {
    Context ctx_;
    BinaryQuery query_;

    async_send_query_params_op(Context ctx, BinaryQuery query)
    : ctx_(std::move(ctx)), query_(std::move(query)) {}

    void perform() {
        decltype(auto) conn = get_connection(ctx_);
        if (auto ec = set_nonblocking(conn)) {
            return done(ctx_, ec);
        }

        if (!send_query_params(conn, query_)) {
            return done(ctx_, error::pg_send_query_params_failed);
        }

        (*this)();
    }

    void operator () (error_code ec = error_code{}, std::size_t = 0) {
        // if data has been flushed or error has been set by
        // read operation no write opertion handling is needed
        // anymore.
        if (get_query_state(ctx_) != query_state::send_in_progress) {
            return;
        }

        // In case of write operation error - finish the request
        // with error.
        if (ec) {
            return done(ctx_, ec);
        }

        // Trying to flush output one more time according to the
        // documentation
        switch (flush_output(get_connection(ctx_))) {
            case query_state::error:
                done(ctx_, error::pg_flush_failed);
                break;
            case query_state::send_in_progress:
                write_poll(ctx_, *this);
                break;
            case query_state::send_finish:
                set_query_state(ctx_, query_state::send_finish);
                break;
        }
    }

    using executor_type = std::decay_t<decltype(asio::get_associated_executor(get_handler(ctx_)))>;

    executor_type get_executor() const noexcept {
        return asio::get_associated_executor(get_handler(ctx_));
    }

    using allocator_type = std::decay_t<decltype(asio::get_associated_allocator(get_handler(ctx_)))>;

    allocator_type get_allocator() const noexcept {
        return asio::get_associated_allocator(get_handler(ctx_));
    }
};

template <typename T, typename Allocator, typename ...Ts>
inline auto make_binary_query(const query_builder<Ts...>& builder, const oid_map_t<T>& m, Allocator a) {
    return binary_query(builder.build(), m, a);
}

template <typename T, typename M, typename Alloc, typename = Require<Query<T>>>
inline auto make_binary_query(const T& query, const M& oid_map, const Alloc& allocator) {
    return binary_query(query, oid_map, allocator);
}

template <typename ...Ts, typename M, typename A>
inline auto make_binary_query(binary_query<Ts...> query, M&&, A&&) {
    return std::move(query);
}

template <typename Context, typename Query>
void async_send_query_params(std::shared_ptr<Context> ctx, Query&& query) {
    auto q = make_binary_query(std::forward<Query>(query),
                        get_oid_map(get_connection(ctx)),
                        asio::get_associated_allocator(get_handler(ctx)));

    async_send_query_params_op op{std::move(ctx), std::move(q)};
    op.perform();
}

#include <boost/asio/yield.hpp>

template <typename Context, typename ResultProcessor>
struct async_get_result_op : boost::asio::coroutine {
    Context ctx_;
    ResultProcessor process_;

    async_get_result_op(Context ctx, ResultProcessor process)
    : ctx_(ctx), process_(process) {}

    void perform() {
        (*this)();
    }

    void done() {
        return impl::done(ctx_);
    }

    void done(error_code ec) {
        if (std::empty(get_error_context(get_connection(ctx_)))) {
            set_error_context(get_connection(ctx_), "error while get request result");
        }
        return impl::done(ctx_, ec);
    }

    void operator() (error_code ec = error_code{}, std::size_t = 0) {
        // In case when query error state has been set by send query params
        // operation skip handle and do nothing more.
        if (get_query_state(ctx_) == query_state::error) {
            return;
        }

        if (ec) {
            // Bad descriptor error can occur here if the connection
            // has been closed by user during processing.
            if (ec == asio::error::bad_descriptor) {
                ec = asio::error::operation_aborted;
            }
            return done(ec);
        }

        reenter(*this) {
            while (is_busy(get_connection(ctx_))) {
                yield read_poll(ctx_, *this);
                if (auto err = consume_input(get_connection(ctx_))) {
                    return done(err);
                }
            }

            set_request_result(ctx_, get_result(get_connection(ctx_)));

            if (!get_request_result(ctx_)) {
                return done();
            }

            if (result_status(*get_request_result(ctx_)) != PGRES_SINGLE_TUPLE) {
                do {
                    while (is_busy(get_connection(ctx_))) {
                        yield read_poll(ctx_, *this);
                        if (consume_input(get_connection(ctx_))) {
                            return handle_result();
                        }
                    }
                } while (get_result(get_connection(ctx_)));
            }

            handle_result();
        }
    }

    void handle_result() {
        const auto status = result_status(*get_request_result(ctx_));
        switch (status) {
            case PGRES_SINGLE_TUPLE:
                process_and_done(std::move(get_request_result(ctx_)));
                return;
            case PGRES_TUPLES_OK:
                process_and_done(std::move(get_request_result(ctx_)));
                return;
            case PGRES_COMMAND_OK:
                done();
                return;
            case PGRES_BAD_RESPONSE:
                done(error::result_status_bad_response);
                return;
            case PGRES_EMPTY_QUERY:
                done(error::result_status_empty_query);
                return;
            case PGRES_FATAL_ERROR:
                done(result_error(*get_request_result(ctx_)));
                return;
            case PGRES_COPY_OUT:
            case PGRES_COPY_IN:
            case PGRES_COPY_BOTH:
            case PGRES_NONFATAL_ERROR:
                break;
        }

        set_error_context(get_connection(ctx_), get_result_status_name(status));
        done(error::result_status_unexpected);
    }

    template <typename Result>
    void process_and_done(Result&& res) noexcept {
        try {
            process_(std::forward<Result>(res), get_connection(ctx_));
        } catch (const std::exception& e) {
            set_error_context(get_connection(ctx_), e.what());
            return done(error::bad_result_process);
        }
        done();
    }

    using executor_type = std::decay_t<decltype(asio::get_associated_executor(get_handler(ctx_)))>;

    executor_type get_executor() const noexcept {
        return asio::get_associated_executor(get_handler(ctx_));
    }

    using allocator_type = std::decay_t<decltype(asio::get_associated_allocator(get_handler(ctx_)))>;

    allocator_type get_allocator() const noexcept {
        return asio::get_associated_allocator(get_handler(ctx_));
    }
};

#include <boost/asio/unyield.hpp>

template <typename Context, typename ResultProcessor>
inline void async_get_result(Context&& ctx, ResultProcessor&& p) {
    async_get_result_op op{std::forward<Context>(ctx), std::forward<ResultProcessor>(p)};
    op.perform();
}

template <typename OutHandler, typename Query, typename TimeConstraint, typename Handler>
struct async_request_op {
    OutHandler out_;
    Query query_;
    TimeConstraint time_constrain_;
    Handler handler_;

    async_request_op(Query query, TimeConstraint time_constrain, OutHandler out, Handler handler)
    : out_(std::move(out)), query_(std::move(query)), time_constrain_(time_constrain), handler_(std::move(handler)) {}

    template <typename Connection>
    void operator() (error_code ec, Connection conn) {
        if (ec) {
            return handler_(ec, std::move(conn));
        }

        auto strand = ozo::detail::make_strand_executor(ozo::get_executor(conn));

        auto ctx = make_request_operation_context(
            std::move(conn),
            asio::bind_executor(strand, detail::bind_cancel_timer<std::decay_t<TimeConstraint>>(
                detail::post_handler(std::move(handler_))
            ))
        );
        detail::set_io_timeout(get_connection(ctx), get_handler(ctx), time_constrain_);

        async_send_query_params(ctx, std::move(query_));
        async_get_result(std::move(ctx), std::move(out_));
    }

    using executor_type = std::decay_t<decltype(asio::get_associated_executor(handler_))>;

    executor_type get_executor() const noexcept {
        return asio::get_associated_executor(handler_);
    }

    using allocator_type = std::decay_t<decltype(asio::get_associated_allocator(handler_))>;

    allocator_type get_allocator() const noexcept {
        return asio::get_associated_allocator(handler_);
    }
};

template <typename T>
struct async_request_out_handler {
    T out;

    async_request_out_handler(T out) : out(std::move(out)) {}

    template <typename Handle, typename Conn>
    void operator() (Handle&& h, Conn& conn) {
        auto res = ozo::make_result(std::forward<Handle>(h));
        ozo::recv_result(res, get_oid_map(conn), out);
    }
};

template <typename P, typename Q, typename TimeConstraint, typename Out, typename Handler>
inline void async_request(P&& provider, Q&& query, TimeConstraint t, Out&& out, Handler&& handler) {
    static_assert(ConnectionProvider<P>, "is not a ConnectionProvider");
    static_assert(Query<Q> || QueryBuilder<Q>, "is neither Query nor QueryBuilder");
    static_assert(ozo::TimeConstraint<TimeConstraint>, "should model TimeConstraint concept");
    async_get_connection(std::forward<P>(provider), deadline(t),
        async_request_op{
            std::forward<Q>(query),
            deadline(t),
            async_request_out_handler{std::forward<Out>(out)},
            std::forward<Handler>(handler)
        }
    );
}

} // namespace impl
} // namespace ozo

