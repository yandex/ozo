#pragma once

#include <ozo/connection.h>
#include <ozo/time_traits.h>

#include <libpq-fe.h>
#include <memory>

namespace ozo {

/**
 * @brief Cancel operation handle
 *
 * This class decoupling the `Connection` object from the `ozo::cancel()` operation to
 * be more concurrent-safe and thread-safe.
 *
 * @ingroup group-requests-types
 */
template <typename Executor>
class cancel_handle {
public:
    using executor_type = std::decay_t<Executor>; //!< Cancel operation Executor type

    executor_type get_executor() const { return std::get<1>(v_);} //!< Executor object cancel operation to run on

    using native_handle_type = PGcancel*; //!< Cancel operation native libpq handle type

    native_handle_type native_handle() const { return std::get<0>(v_).get();} //!< Cancel operation native libpq handle

    cancel_handle(native_handle_type handle, Executor ex) : v_(handle, std::move(ex)) {}

private:
    struct deleter {
        void operator()(native_handle_type h) const { PQfreeCancel(h); }
    };

    std::tuple<std::unique_ptr<PGcancel, deleter>, executor_type> v_;
};

/**
 * @brief Get cancel handle for the cancel operation
 *
 * @param connection --- `Connection` with active operation to cancel.
 * @param executor   --- Executor on which the operation will be executed. Default executor is
 *                       `boost::asio::system_executor`.
 *
 * @return null-state object if given connection is in bad state.
 * @return initialized object if given connection is in good state.
 *
 * Since libpq's cancel operation implementation is synchronous only it would block a thread where executed.
 * That's why it needs a dedicated Executor. User should specify an executor to implement a proper execution
 * strategy, e.g. the operations' queue which would be handled in a dedicated thread and so on.
 *
 * @ingroup group-requests-functions
 */
template <typename Connection, typename Executor = boost::asio::system_executor>
inline cancel_handle<Executor> get_cancel_handle(const Connection& connection, Executor&& executor = Executor{});

#ifdef OZO_DOCUMENTATION


/**
 * @brief Cancel execution of current request on a database backend.
 *
 * Sometimes you need to cancel request to a database due to operation time constraint.
 * In case of closing connection, PostgreSQL backend will continue to execute request anyway.
 * So to prevent such waste of database resources it is best practice to cancel the execution
 * of the request by sending special command.
 *
 * @note The function does not particitate in ADL since could be implemented via functional object.
 * @note If a timer hits the specified time constraint only waiting process would be canceled.
 * The cancel operation itself would continue to execute since there is no way to cancel it.
 * User should take it into account planning to use specified executor.
 * @note It is not recommended to use cancel() with external connection poolers like pgbouncer or
 * [Odyssey](https://github.com/yandex/odyssey) out of explicit transaction (with autocommit feature),
 * since it could lead to a canceling of concurrent query, which shares the same backend.
 *
 * @param handle --- handle for the cancel operation, should be obtained via `ozo::get_cancel_handle`.
 * @param io --- io_context on which the operation timer will run.
 * @param time_constraint --- time constraint to wait for the operation complete.
 * @param token --- valid `CancelCompletionToken` which is described below.
 * @return deduced from `CancelCompletionToken`.
 *
 * ### CancelCompletionToken
 *
 * It determines how to continue with an asynchronous cancel operation result when the operation is complete.
 *
 * Valid `CancelCompletionToken` is one of:
 * * Callback with signature `void(ozo::error_code ec, std::string error_message)`. In this case `ozo::cancel()` returns `void`.
 *   First argument provides error code, second - error message from the undelying `libpq` library.
 * * [`boost::asio::use_future`](https://www.boost.org/doc/libs/1_66_0/doc/html/boost_asio/reference/use_future.html) ---
 *   to get a future on the asynchronous operation result. In this case `ozo::cancel()` will return `std::future<void>`. Additional
 *   error message from the undelying `libpq` library will be lost. For this reason the completion token is not recommended for
 *   use in production code. In case of synchronization is needed it is better to use a callback with promise and future are
 *   used directely.
 * * [`boost::asio::yield_context`](https://www.boost.org/doc/libs/1_66_0/doc/html/boost_asio/reference/basic_yield_context.html) ---
 *   to use async operation with Boost.Coroutine. Asynchronous function in this case will return `std::string`.
 *   @note To get error message from the undelying `libpq` library error code
 *   [binding](https://www.boost.org/doc/libs/1_66_0/doc/html/boost_asio/reference/basic_yield_context/operator_lb__rb_.html)
 *   should be used to avoid exception in case of error.
 * * any other type supported by [`boost::asio::async_result`](https://www.boost.org/doc/libs/1_66_0/doc/html/boost_asio/reference/async_result.html).
 *
 * ### Example
 * @code
ozo::io_context io;
boost::asio::steady_timer timer(io);

boost::asio::spawn(io, [&io, &timer](auto yield){
    const ozo::connection_info conn_info(OZO_PG_TEST_CONNINFO);
    ozo::error_code ec;
    auto conn = ozo::get_connection(conn_info[io], yield[ec]);
    assert(!ec);

    // We want to cancel query if it takes too long
    boost::asio::spawn(yield, [&io, &timer, handle = get_cancel_handle(conn)](auto yield) mutable {
        // Wait a second for the request result
        timer.expires_after(1s);
        timer.async_wait(yield);

        // In this case guard is needed since cancel will be served with
        // external system executor and we need to protect our io_context
        // from stop until cancel continuation will be called. It is not
        // necessary if you already protect the io service with a work guard.
        auto guard = boost::asio::make_work_guard(io);
        // Cancel should be performed within 5 seconds, othervise the
        // coroutine will be released.
        ozo::error_code ec;
        auto error_msg = ozo::cancel(std::move(handle), io, 5s, yield[ec]);
        if (ec) {
            std::cerr << ec.message() << ", " << error_msg << std::endl;
        }
    });
    ozo::execute(conn, "SELECT pg_sleep(1000000)"_SQL, yield[ec]);
    assert(ec == ozo::sqlstate::query_canceled);
});

io.run();
 * @endcode
 * @ingroup group-requests-functions
 */
template <typename Executor, typename TimeConstraint, typename CancelCompletionToken>
auto cancel(cancel_handle<Executor>&& handle, io_context& io, TimeConstraint time_constraint, CancelCompletionToken&& token);

/**
 * @brief Cancel execution of current request on a database backend.
 *
 * This version executes cancel operation without a time constraint.
 *
 * @note The function does not particitate in ADL since could be implemented via functional object.
 *
 * @param handle --- handle for the cancel operation, should be obtained via `ozo::get_cancel_handle`.
 * @param token --- valid `CancelCompletionToken` (see time constraint version of `ozo::cancel()` for details).
 * @return deduced from `CancelCompletionToken`.
 *
 * @warning Use it carefully because you can't cancel this `cancel()` operation. The only option is to stop wait.
 *
 * @ingroup group-requests-functions
 */
template <typename Executor, typename CancelCompletionToken>
auto cancel(cancel_handle<Executor>&& handle, CancelCompletionToken&& token);
#else
struct cancel_op {
    template <typename Executor, typename TimeConstraint, typename CancelCompletionToken>
    auto operator() (cancel_handle<Executor>&& handle, io_context& io,
        TimeConstraint time_constraint, CancelCompletionToken&& token) const;

    template <typename Executor, typename CancelCompletionToken>
    auto operator() (cancel_handle<Executor>&& handle, CancelCompletionToken&& token) const;
};

constexpr cancel_op cancel;

#endif
} // namespace ozo

#include <ozo/impl/cancel.h>
