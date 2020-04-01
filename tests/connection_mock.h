#pragma once

#include "test_asio.h"

#include <ozo/impl/io.h>
#include <ozo/impl/transaction.h>
#include <ozo/time_traits.h>

namespace ozo::tests {

struct pg_result {
    ExecStatusType status;
    const char* error;
};

struct PGconn_mock {
    PGconn_mock() {
        using testing::_;
        ON_CALL(*this, PQsocket()).WillByDefault(::testing::Return(-1));
        ON_CALL(*this, PQstatus()).WillByDefault(::testing::Return(CONNECTION_BAD));
        ON_CALL(*this, PQtransactionStatus()).WillByDefault(::testing::Return(PQTRANS_UNKNOWN));
        ON_CALL(*this, PQflush()).WillByDefault(::testing::Return(-1));
        ON_CALL(*this, PQsetnonblocking(testing::_)).WillByDefault(::testing::Return(-1));
        ON_CALL(*this, PQisBusy()).WillByDefault(::testing::Return(1));
        ON_CALL(*this, PQconsumeInput()).WillByDefault(::testing::Return(0));
        ON_CALL(*this, PQconnectPoll()).WillByDefault(::testing::Return(PGRES_POLLING_FAILED));
        ON_CALL(*this, PQsendQueryParams(_, _, _, _, _, _, _)).WillByDefault(::testing::Return(0));
    };

    MOCK_METHOD0(PQsocket, int());
    friend int PQsocket(PGconn_mock* self) {
        return mock(self).PQsocket();
    }

    MOCK_METHOD0(PQstatus, ConnStatusType());
    friend ConnStatusType PQstatus(PGconn_mock* self) {
        return mock(self).PQstatus();
    }

    MOCK_METHOD0(PQtransactionStatus, PGTransactionStatusType());
    friend PGTransactionStatusType PQtransactionStatus(PGconn_mock* self) {
        return mock(self).PQtransactionStatus();
    }

    MOCK_METHOD0(PQflush, int());
    friend int PQflush(PGconn_mock* self) {
        return mock(self).PQflush();
    }

    MOCK_METHOD1(PQsetnonblocking, int(int));
    friend int PQsetnonblocking(PGconn_mock* self, int v) {
        return mock(self).PQsetnonblocking(v);
    }

    MOCK_METHOD0(PQisBusy, int());
    friend int PQisBusy(PGconn_mock* self) {
        return mock(self).PQisBusy();
    }

    MOCK_METHOD0(PQconsumeInput, int());
    friend int PQconsumeInput(PGconn_mock* self) {
        return mock(self).PQconsumeInput();
    }

    MOCK_METHOD0(PQconnectPoll, int());
    friend int PQconnectPoll(PGconn_mock* self) {
        return mock(self).PQconnectPoll();
    }

    MOCK_METHOD7(PQsendQueryParams, int(
                      const char*, int, const Oid*,
                      const char* const*, const int*,
                      const int*, int));
    friend int PQsendQueryParams(PGconn_mock* self,
                      const char *command,
                      int nParams,
                      const Oid *paramTypes,
                      const char * const *paramValues,
                      const int *paramLengths,
                      const int *paramFormats,
                      int resultFormat) {
        return mock(self).PQsendQueryParams(
            command, nParams, paramTypes, paramValues,
            paramLengths, paramFormats, resultFormat
        );
    }

    MOCK_METHOD0(PQgetResult, pg_result*());
    friend pg_result* PQgetResult(PGconn_mock* self) {
        return mock(self).PQgetResult();
    }

private:
    static PGconn_mock& mock(PGconn_mock* self) { return self ? *self : null_mock();}
    static PGconn_mock& null_mock() {
        using testing::_;
        static PGconn_mock mock;
        // These defaults are copy-pasted here to to get a line number relates to the null_mock()
        // form the gmock console warning message. If you are here - it looks like your
        // PGconn_mock pointer is nullptr and if it is on purpose then you can ignore this messages.
        ON_CALL(mock, PQsocket()).WillByDefault(::testing::Return(-1));
        ON_CALL(mock, PQstatus()).WillByDefault(::testing::Return(CONNECTION_BAD));
        ON_CALL(mock, PQtransactionStatus()).WillByDefault(::testing::Return(PQTRANS_UNKNOWN));
        ON_CALL(mock, PQflush()).WillByDefault(::testing::Return(-1));
        ON_CALL(mock, PQsetnonblocking(testing::_)).WillByDefault(::testing::Return(-1));
        ON_CALL(mock, PQisBusy()).WillByDefault(::testing::Return(1));
        ON_CALL(mock, PQconsumeInput()).WillByDefault(::testing::Return(0));
        ON_CALL(mock, PQconnectPoll()).WillByDefault(::testing::Return(PGRES_POLLING_FAILED));
        ON_CALL(mock, PQsendQueryParams(_, _, _, _, _, _, _)).WillByDefault(::testing::Return(0));
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

inline decltype(auto) PQresultStatus(const pg_result* res) noexcept {
    return res->status;
}

inline const char* PQresultErrorField(const pg_result* res, int) noexcept {
    return res->error;
}

using ozo::empty_oid_map;

struct cancel_handle_mock {
    MOCK_METHOD0(dispatch_cancel, std::tuple<error_code, std::string>());

    friend auto dispatch_cancel(cancel_handle_mock* self) {
        return self->dispatch_cancel();
    }
};

struct connection_mock {
    MOCK_METHOD0(cancel, void());
    MOCK_CONST_METHOD0(is_bad, bool());
    MOCK_METHOD0(close, ozo::error_code());
    MOCK_METHOD1(async_wait_write, void(std::function<void(error_code)>));
    MOCK_METHOD1(async_wait_read, void(std::function<void(error_code)>));

    MOCK_METHOD1(start_connection, native_conn_handle(const std::string&));
    MOCK_METHOD0(assign, ozo::error_code());
    MOCK_METHOD0(async_request, void());
    MOCK_METHOD0(async_execute, void());
    MOCK_METHOD0(request_oid_map, void());
    MOCK_METHOD0(get_cancel_handle, cancel_handle_mock*());
};

using connection_gmock = connection_mock;

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

namespace pg {

template<>
struct safe_handle<ozo::tests::pg_result> {
    using type = ozo::tests::pg_result*;
};
} // namespace pg
} // namespace ozo

namespace ozo::tests {

static_assert(Query<fake_query>, "fake_query is not a Query");

template <typename OidMap = empty_oid_map>
struct connection {
    using handle_type = ozo::tests::native_conn_handle;
    using native_handle_type = ozo::tests::PGconn_mock*;
    using error_context_type = std::string;
    using oid_map_type = OidMap;
    using executor_type = io_context::executor_type;

    handle_type handle_;
    OidMap oid_map_;
    connection_mock* mock_ = nullptr;
    error_context_type error_context_;
    io_context* io_;

    connection(handle_type handle, OidMap oid_map, connection_mock* mock, error_context_type error_context_type, io_context* io)
    : handle_(std::move(handle)), oid_map_(oid_map), mock_(mock), error_context_(error_context_type), io_(io) {}

    executor_type get_executor() const { return io_->get_executor(); }

    auto native_handle() const noexcept { return handle_.get(); }

    const error_context_type& get_error_context() const noexcept { return error_context_; }

    void set_error_context(error_context_type v = error_context_type{}) { error_context_ = std::move(v); }

    oid_map_type& oid_map() noexcept { return oid_map_;}

    const oid_map_type& oid_map() const noexcept { return oid_map_;}

    bool is_bad() const noexcept { return mock_->is_bad();}

    operator bool () const noexcept { return !is_bad();}

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

    ozo::error_code close() { return mock_->close(); }

    native_conn_handle release() {
        return std::move(handle_);
    }

    void cancel() { mock_->cancel(); }

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

    template <typename WaitHandler>
    void async_wait_write(WaitHandler&& h) {
        mock_->async_wait_write([h = std::forward<WaitHandler>(h)] (auto e) {
            asio::post(ozo::detail::bind(std::move(h), std::move(e)));
        });
    }

    template <typename WaitHandler>
    void async_wait_read(WaitHandler&& h) {
        mock_->async_wait_read([h = std::forward<WaitHandler>(h)] (auto e) {
            asio::post(ozo::detail::bind(std::move(h), std::move(e)));
        });
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
        PGconn_mock& handle, OidMap oid_map = OidMap{}) {
    return std::make_shared<connection<OidMap>>(connection<OidMap>{
            std::addressof(handle),
            oid_map,
            std::addressof(mock),
            "",
            std::addressof(io)
        });
}

} // namespace ozo::tests
