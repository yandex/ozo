#pragma once

#include "test_asio.h"

#include <ozo/impl/io.h>
#include <ozo/impl/transaction.h>
#include <ozo/time_traits.h>

namespace ozo::tests {

struct PGconn_mock {
    PGconn_mock() {
        ON_CALL(*this, PQsocket()).WillByDefault(::testing::Return(-1));
        ON_CALL(*this, PQstatus()).WillByDefault(::testing::Return(CONNECTION_BAD));
        ON_CALL(*this, PQtransactionStatus()).WillByDefault(::testing::Return(PQTRANS_UNKNOWN));
    };

    MOCK_METHOD0(PQsocket, int());
    friend int PQsocket(PGconn_mock* self) {
        return self ? self->PQsocket() : null_mock().PQsocket();
    }

    MOCK_METHOD0(PQstatus, ConnStatusType());
    friend ConnStatusType PQstatus(PGconn_mock* self) {
        return self ? self->PQstatus() : null_mock().PQstatus();
    }

    MOCK_METHOD0(PQtransactionStatus, PGTransactionStatusType());
    friend PGTransactionStatusType PQtransactionStatus(PGconn_mock* self) {
        return self ? self->PQtransactionStatus() : null_mock().PQtransactionStatus();
    }

private:
    static PGconn_mock& null_mock() {
        static PGconn_mock mock;
        // These defaults copy-pasted here to to get a line number relates to the null_mock()
        // form the gmock console warning message. If you are here - it looks like your
        // PGconn_mock pointer is nullptr and if it is on purpose you can ignore this messages.
        ON_CALL(mock, PQsocket()).WillByDefault(::testing::Return(-1));
        ON_CALL(mock, PQstatus()).WillByDefault(::testing::Return(CONNECTION_BAD));
        ON_CALL(mock, PQtransactionStatus()).WillByDefault(::testing::Return(PQTRANS_UNKNOWN));
        return mock;
    }
};

struct native_conn_handle {
    PGconn_mock* mock_ = nullptr;

    using pointer = PGconn_mock*;

    native_conn_handle(pointer mock = nullptr) : mock_(mock) {}

    auto& operator * () const { assert_not_null(); return *mock_; }
    pointer operator -> () const { assert_not_null(); return mock_; }
    pointer get () const { assert_not_null(); return mock_; }
    operator bool () const { return mock_ != nullptr; }

    void assert_not_null() const {
        if (!mock_) {
            throw std::invalid_argument("ozo::tests::native_conn_handle is in null state");
        }
    }
};

struct pg_result {
    ExecStatusType status;
    error_code error;
};

inline decltype(auto) pq_result_status(const pg_result& res) noexcept {
    return res.status;
}

inline error_code pq_result_error(const pg_result& res) noexcept {
    return res.error;
}

using ozo::empty_oid_map;

struct cancel_handle_mock {
    MOCK_METHOD0(dispatch_cancel, std::tuple<error_code, std::string>());

    friend auto dispatch_cancel(cancel_handle_mock* self) {
        return self->dispatch_cancel();
    }
};

struct connection_mock {
    MOCK_METHOD0(set_nonblocking, int());
    MOCK_METHOD0(send_query_params, int());
    MOCK_METHOD0(consume_input, int());
    MOCK_CONST_METHOD0(is_busy, bool());
    MOCK_METHOD0(flush_output, ozo::impl::query_state());
    MOCK_METHOD0(get_result, boost::optional<pg_result>());

    MOCK_CONST_METHOD0(connect_poll, int());
    MOCK_METHOD1(start_connection, native_conn_handle(const std::string&));
    MOCK_METHOD0(assign, ozo::error_code());
    MOCK_METHOD0(async_request, void());
    MOCK_METHOD0(async_execute, void());
    MOCK_METHOD0(request_oid_map, void());
    MOCK_METHOD0(set_executor, ozo::error_code());
    MOCK_METHOD0(get_cancel_handle, cancel_handle_mock*());
};

using connection_gmock = connection_mock;

inline boost::optional<pg_result> make_pg_result(
        ExecStatusType status, error_code error) {
    return pg_result{status, error};
}

struct fake_query {
    hana::tuple<> params;
};

} // namespace ozo::tests

namespace ozo {

template <>
struct get_query_text_impl<tests::fake_query> {
    static constexpr decltype(auto) apply(const tests::fake_query&) noexcept {
        return "fake query";
    }
};

template <>
struct get_query_params_impl<tests::fake_query> {
    static constexpr decltype(auto) apply(const tests::fake_query& q) noexcept {
        return q.params;
    }
};

} // namespace ozo

namespace ozo::tests {

static_assert(Query<fake_query>, "fake_query is not a Query");

template <typename OidMap = empty_oid_map>
struct connection {
    using handle_type = ozo::tests::native_conn_handle;
    using native_handle_type = ozo::tests::PGconn_mock*;
    using error_context = std::string;
    using oid_map_type = OidMap;
    using executor_type = io_context::executor_type;

    handle_type handle_;
    stream_descriptor socket_;
    OidMap oid_map_;
    connection_mock* mock_ = nullptr;
    error_context error_context_;
    io_context* io_;

    executor_type get_executor() const { return io_->get_executor(); }

    auto native_handle() const noexcept { return handle_.get(); }

    const error_context& get_error_context() const noexcept { return error_context_; }

    void set_error_context(error_context v = error_context{}) { error_context_ = std::move(v); }

    oid_map_type& oid_map() noexcept { return oid_map_;}

    const oid_map_type& oid_map() const noexcept { return oid_map_;}

    bool is_bad() const noexcept {
        return ozo::detail::connection_status_bad(native_handle());
    }

    operator bool () const noexcept { return !is_bad();}

    friend int pq_set_nonblocking(connection& c) {
        return c.mock_->set_nonblocking();
    }

    template <typename ...Ts>
    friend int pq_send_query_params(connection& c, Ts&&...) noexcept {
        return c.mock_->send_query_params();
    }

    friend int pq_consume_input(connection& c) noexcept {
        return c.mock_->consume_input();
    }

    friend bool pq_is_busy(connection& c) noexcept {
        return c.mock_->is_busy();
    }

    friend ozo::impl::query_state pq_flush_output(connection& c) noexcept {
        return c.mock_->flush_output();
    }

    friend decltype(auto) pq_get_result(connection& c) noexcept {
        return c.mock_->get_result();
    }

    friend int pq_connect_poll(connection& c) {
        return c.mock_->connect_poll();
    }

    friend handle_type pq_start_connection(
            connection& c, const std::string& conninfo) {
        return c.mock_->start_connection(conninfo);
    }

    ozo::error_code assign(handle_type&& handle) {
        ozo::error_code ec = mock_->assign();
        if (!ec) {
            handle_ = std::move(handle);
        }
        return ec;
    }

    ozo::error_code close() {
        error_code ec;
        socket_.close(ec);
        return ec;
    }

    native_conn_handle release() {
        return std::move(handle_);
    }

    void cancel() {
        ozo::error_code _;
        socket_.cancel(_);
    }

    friend decltype(auto) get_cancel_handle(connection& c) {
        return c.mock_->get_cancel_handle();
    }

    template <typename Q, typename Out, typename Handler>
    friend void async_request(std::shared_ptr<connection>&& provider, Q&&, const ozo::time_traits::duration&, Out&&, Handler&&) {
        provider->mock_->async_request();
    }

    template <typename Q, typename Handler>
    friend void async_execute(std::shared_ptr<connection>& provider, Q&&, const ozo::time_traits::duration&, Handler&&) {
        provider->mock_->async_execute();
    }

    template <typename Handler>
    friend void request_oid_map(std::shared_ptr<connection>&& provider, Handler&&) {
        provider->mock_->request_oid_map();
    }

    template <typename Q, typename Options, typename Handler>
    friend void async_execute(ozo::impl::transaction<std::shared_ptr<connection>, Options>&& transaction, Q&&,
            const ozo::time_traits::duration&, Handler&&) {
        std::shared_ptr<connection> connection;
        transaction.take_connection(connection);
        connection->mock_->async_execute();
    }

    template <typename Executor>
    ozo::error_code set_executor(const Executor&) {
        return mock_->set_executor();
    }

    template <typename WaitHandler>
    void async_wait_write(WaitHandler&& h) {
        socket_.async_write_some(asio::null_buffers(), std::forward<WaitHandler>(h));
    }

    template <typename WaitHandler>
    void async_wait_read(WaitHandler&& h) {
        socket_.async_read_some(asio::null_buffers(), std::forward<WaitHandler>(h));
    }
};

} // namespace ozo::tests

namespace ozo {

template <typename OidMap>
struct is_connection<tests::connection<OidMap>> : std::true_type {};

} // namespace ozo

namespace ozo::tests {

template <typename ...Ts>
using connection_ptr = std::shared_ptr<connection<Ts...>>;

static_assert(ozo::Connection<connection<>>,
    "connection does not meet Connection requirements");
static_assert(ozo::Connection<connection<>>,
    "connection does not meet Connection requirements");
static_assert(ozo::Connection<connection_ptr<>>,
    "connection_ptr does not meet Connection requirements");

template <typename OidMap = empty_oid_map>
inline auto make_connection(connection_mock& mock, io_context& io,
        stream_descriptor_mock& socket_mock, PGconn_mock& handle, OidMap oid_map) {
    return std::make_shared<connection<OidMap>>(connection<OidMap>{
            std::addressof(handle),
            stream_descriptor{io, socket_mock},
            oid_map,
            std::addressof(mock),
            "",
            std::addressof(io)
        });
}

template <typename OidMap = empty_oid_map>
inline auto make_connection(connection_mock& mock, io_context& io,
        stream_descriptor_mock& socket_mock, OidMap oid_map = OidMap{}) {
    return std::make_shared<connection<OidMap>>(connection<OidMap>{
            nullptr,
            stream_descriptor{io, socket_mock},
            oid_map,
            std::addressof(mock),
            "",
            std::addressof(io)
        });
}

} // namespace ozo::tests
