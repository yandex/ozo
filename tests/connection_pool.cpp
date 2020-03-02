#include "connection_mock.h"
#include "test_error.h"

#include <ozo/connection_info.h>
#include <ozo/connection_pool.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace {

TEST(make_connection_pool, should_not_throw) {
    boost::asio::io_context io;
    ozo::connection_info conn_info("conn info string");
    const ozo::connection_pool_config config;
    EXPECT_NO_THROW(ozo::make_connection_pool(conn_info, config));
}

} //namespace

namespace ozo::tests {

struct pool_handle_mock {
    struct value_type {
        using native_handle_type = PGconn_mock*;
        using oid_map_type = empty_oid_map;
        using error_context_type = std::string;
        using statistics_type = ozo::none_t;

        native_conn_handle safe_handle_;
        ozo::empty_oid_map oid_map_;
        error_context_type error_context_;

        const native_conn_handle& safe_native_handle() const & {return safe_handle_;}
        native_conn_handle& safe_native_handle() & {return safe_handle_;}

        const oid_map_type& oid_map() const & {return oid_map_;}

        const statistics_type& statistics() const & {return ozo::none;}
        template <typename Key, typename Value>
        void update_statistics(const Key&, Value&&) noexcept {
            static_assert(std::is_void_v<Key>, "update_statistics is not supperted");
        }
        const error_context_type& get_error_context() const noexcept {
            return error_context_;
        }
        void set_error_context(error_context_type v) {
            error_context_ = std::move(v);
        }
    };

    MOCK_CONST_METHOD0(empty, bool());
    MOCK_METHOD1(reset, void(value_type&));
    MOCK_METHOD0(waste, void());
    MOCK_METHOD0(value, value_type&());

    virtual ~pool_handle_mock() = default;
};

struct connection_pool {
    struct handle {
        pool_handle_mock* mock_;
        using value_type = pool_handle_mock::value_type;
        using native_handle_type = value_type::native_handle_type;
        using oid_map_type = value_type::oid_map_type;
        using error_context_type = value_type::error_context_type;
        using statistics_type = value_type::statistics_type;

        handle(const handle&) = delete;
        handle(handle&&) = default;
        handle& operator = (const handle& ) = delete;
        handle& operator = (handle&& ) = default;
        bool empty() const { return mock_->empty(); }
        void reset(value_type v) { mock_->reset(v); }
        void waste() { mock_->waste(); }
        value_type& operator * () { return mock_->value(); }
        const value_type& operator * () const { return mock_->value(); }
        value_type* operator -> () { return std::addressof(mock_->value()); }
        const value_type* operator -> () const { return std::addressof(mock_->value()); }
    };
};

struct connection_source_mock {
    using connection_type = std::shared_ptr<connection<>>;
    using handler_type = std::function<void(error_code, connection_type)>;
    MOCK_METHOD1(async_get_connection, void(handler_type));

    virtual ~connection_source_mock() = default;
};

struct connection_source {
    connection_source_mock* mock_ = nullptr;

    using connection_type = std::shared_ptr<connection<>>;
    using source_type = connection_source;

    template <typename IoContext, typename TimeConstraint, typename Handler>
    void operator()(IoContext&, TimeConstraint&&, Handler&& h) const {
        mock_->async_get_connection(ozo::detail::make_copyable(std::forward<Handler>(h)));
    }
};
} // namespace ozo::tests

namespace ozo {
template <>
class connection_pool<ozo::tests::connection_source> {
public:
    using connection_type = std::shared_ptr<ozo::pooled_connection<ozo::tests::connection_pool::handle, ozo::tests::executor>>;
};

template <>
struct unwrap_impl<ozo::tests::connection_pool::handle> {
    template <typename T>
    static constexpr decltype(auto) apply(T&& handle) {
        return *handle;
    }
};

namespace detail {
template <>
struct connection_stream<ozo::tests::executor> {
    using type = ozo::tests::stream_descriptor;

    static type get(const ozo::tests::executor& ex, type::native_handle_type fd) {
        return type{ex.context(), fd};
    }

    static type get(const ozo::tests::executor& ex) {
        return type{ex.context()};
    }
};
} // namespace detail
} // namespace ozo

namespace {

using namespace ozo::tests;
using namespace testing;

struct pooled_connection : Test {
    StrictMock<pool_handle_mock> handle_mock{};
    StrictMock<stream_descriptor_mock> stream;
    io_context io;
    StrictMock<stream_descriptor_mock> socket {};

    StrictMock<PGconn_mock> conn_handle;
    pool_handle_mock::value_type value{std::addressof(conn_handle), ozo::empty_oid_map{}, ""};

    using impl = ozo::pooled_connection<ozo::tests::connection_pool::handle, ozo::tests::executor>;

};

TEST_F(pooled_connection, should_call_handle_waste_on_destruction_if_handle_is_not_empty_and_connection_is_bad) {
    EXPECT_CALL(handle_mock, value()).WillRepeatedly(ReturnRef(value));
    EXPECT_CALL(handle_mock, empty()).WillRepeatedly(Return(false));
    EXPECT_CALL(conn_handle, PQsocket()).WillOnce(Return(42));
    EXPECT_CALL(io.stream_service_, create()).WillRepeatedly(ReturnRef(socket));
    EXPECT_CALL(socket, assign(42));
    EXPECT_CALL(conn_handle, PQstatus()).WillOnce(Return(CONNECTION_BAD));
    EXPECT_CALL(socket, release()).WillOnce(Return(42));
    EXPECT_CALL(handle_mock, waste()).WillOnce(Return());

    {
        impl p(io.get_executor(), connection_pool::handle{&handle_mock});
    }
}

namespace with_params {

struct pooled_connection : ::pooled_connection,
        testing::WithParamInterface<PGTransactionStatusType> {
};

TEST_P(pooled_connection, should_call_handle_waste_on_destruction_if_connection_is_good_and_not_idle) {
    EXPECT_CALL(handle_mock, value()).WillRepeatedly(ReturnRef(value));
    EXPECT_CALL(handle_mock, empty()).WillRepeatedly(Return(false));
    EXPECT_CALL(conn_handle, PQsocket()).WillOnce(Return(42));
    EXPECT_CALL(io.stream_service_, create()).WillRepeatedly(ReturnRef(socket));
    EXPECT_CALL(socket, assign(42));
    EXPECT_CALL(conn_handle, PQstatus()).WillOnce(Return(CONNECTION_OK));
    EXPECT_CALL(conn_handle, PQtransactionStatus()).WillOnce(Return(GetParam()));
    EXPECT_CALL(socket, release()).WillOnce(Return(42));
    EXPECT_CALL(handle_mock, waste()).WillOnce(Return());

    {
        impl p(io.get_executor(), connection_pool::handle{&handle_mock});
    }
}

INSTANTIATE_TEST_SUITE_P(
    with_not_PQTRANS_IDLE_transaction_status,
    pooled_connection,
    testing::Values(
        PQTRANS_UNKNOWN,
        PQTRANS_ACTIVE,
        PQTRANS_INTRANS,
        PQTRANS_INERROR
    )
);

} // namespace with_params

TEST_F(pooled_connection, should_not_call_waste_on_destruction_if_handle_is_not_empty_connection_is_good_and_idle) {
    EXPECT_CALL(handle_mock, value()).WillRepeatedly(ReturnRef(value));
    EXPECT_CALL(handle_mock, empty()).WillRepeatedly(Return(false));
    EXPECT_CALL(conn_handle, PQsocket()).WillOnce(Return(42));
    EXPECT_CALL(io.stream_service_, create()).WillRepeatedly(ReturnRef(socket));
    EXPECT_CALL(socket, assign(42));
    EXPECT_CALL(conn_handle, PQstatus()).WillOnce(Return(CONNECTION_OK));
    EXPECT_CALL(conn_handle, PQtransactionStatus()).WillOnce(Return(PQTRANS_IDLE));
    EXPECT_CALL(socket, release()).WillOnce(Return(42));

    {
        impl p(io.get_executor(), connection_pool::handle{&handle_mock});
    }
}

TEST_F(pooled_connection, should_not_check_connection_status_and_call_waste_on_destruction_if_handle_is_empty) {
    EXPECT_CALL(handle_mock, value()).WillRepeatedly(ReturnRef(value));
    EXPECT_CALL(conn_handle, PQsocket()).WillOnce(Return(42));
    EXPECT_CALL(io.stream_service_, create()).WillRepeatedly(ReturnRef(socket));
    EXPECT_CALL(socket, assign(42));
    EXPECT_CALL(socket, release()).WillOnce(Return(42));
    EXPECT_CALL(handle_mock, empty()).WillRepeatedly(Return(false));

    {
        impl p(io.get_executor(), connection_pool::handle{&handle_mock});

        EXPECT_CALL(handle_mock, empty()).WillRepeatedly(Return(true));
    }
}

struct pooled_connection_wrapper : Test {
    using pooled_connection_ptr = std::shared_ptr<ozo::pooled_connection<ozo::tests::connection_pool::handle, ozo::tests::executor>>;
    StrictMock<connection_source_mock> provider_mock;
    StrictMock<callback_gmock<pooled_connection_ptr>> callback_mock;
    StrictMock<connection_gmock> connection_mock{};
    StrictMock<pool_handle_mock> handle_mock{};
    StrictMock<stream_descriptor_mock> stream;
    StrictMock<PGconn_mock> native_handle;
    pool_handle_mock::value_type rep{std::addressof(native_handle), {}, {}};
    io_context io;

    pooled_connection_wrapper() {
        EXPECT_CALL(handle_mock, value()).WillRepeatedly(ReturnRef(rep));
    }

    auto wrap_pooled_connection_handler() {
        return ozo::detail::wrap_pooled_connection_handler(
            io.get_executor(),
            connection_source{&provider_mock},
            ozo::none,
            wrap(callback_mock)
        );
    }

    auto make_connection() {
        return std::make_shared<ozo::tests::connection<>>(connection<>{
            std::addressof(native_handle), {io, stream}, {}, &connection_mock, {}, &io});
    }
};

using ozo::error_code;

TEST_F(pooled_connection_wrapper, should_be_copyable_with_non_copyable_handler_for_resource_pool_compatibility) {
    auto h = ozo::detail::wrap_pooled_connection_handler(
            io.get_executor(),
            connection_source{&provider_mock},
            ozo::none,
            [&, attr = std::unique_ptr<int>()](error_code ec, auto& conn) mutable { callback_mock.call(ec, conn); }
        );

    EXPECT_TRUE(std::is_copy_constructible_v<decltype(h)>);
}

TEST_F(pooled_connection_wrapper, should_invoke_handler_with_error_if_error_is_passed) {
    auto h = wrap_pooled_connection_handler();

    EXPECT_CALL(callback_mock, call(error_code(error::error), _)).WillOnce(Return());

    h(error::error, connection_pool::handle{});
}

TEST_F(pooled_connection_wrapper, should_invoke_handler_if_passed_connection_is_good_and_handle_is_not_empty) {
    auto h = wrap_pooled_connection_handler();


    EXPECT_CALL(handle_mock, empty()).WillRepeatedly(Return(false));
    EXPECT_CALL(io.stream_service_, create()).WillOnce(ReturnRef(stream));
    EXPECT_CALL(stream, assign(42));
    EXPECT_CALL(stream, release());
    EXPECT_CALL(native_handle, PQsocket()).WillRepeatedly(Return(42));
    EXPECT_CALL(native_handle, PQstatus()).WillRepeatedly(Return(CONNECTION_OK));
    EXPECT_CALL(native_handle, PQtransactionStatus()).WillRepeatedly(Return(PQTRANS_IDLE));

    EXPECT_CALL(callback_mock, call(_, _)).WillOnce(Return());

    h({}, connection_pool::handle{&handle_mock});
}


TEST_F(pooled_connection_wrapper, should_call_async_get_connection_and_invoke_handler_if_passed_connection_is_bad_and_handle_is_not_empty) {
    auto h = wrap_pooled_connection_handler();

    Sequence s;
    EXPECT_CALL(handle_mock, empty()).WillRepeatedly(Return(false));
    EXPECT_CALL(native_handle, PQstatus())
        .InSequence(s)
        .WillOnce(Return(CONNECTION_BAD));
    EXPECT_CALL(provider_mock, async_get_connection(_))
        .InSequence(s)
        .WillOnce(InvokeArgument<0>(error_code{}, make_connection()));
    EXPECT_CALL(handle_mock, reset(_)).InSequence(s);
    EXPECT_CALL(io.stream_service_, create()).InSequence(s).WillOnce(ReturnRef(stream));
    EXPECT_CALL(native_handle, PQsocket()).InSequence(s).WillOnce(Return(42));
    EXPECT_CALL(stream, assign(42)).InSequence(s);

    EXPECT_CALL(callback_mock, call(ozo::error_code{}, _))
        .InSequence(s)
        .WillOnce(Return());

    EXPECT_CALL(stream, release()).InSequence(s);
    EXPECT_CALL(native_handle, PQstatus())
        .InSequence(s)
        .WillOnce(Return(CONNECTION_OK));
    EXPECT_CALL(native_handle, PQtransactionStatus())
        .InSequence(s)
        .WillOnce(Return(PQTRANS_IDLE));

    h({}, connection_pool::handle{&handle_mock});
}

TEST_F(pooled_connection_wrapper, should_call_async_get_connection_and_invoke_handler_if_handle_is_empty) {
    auto h = wrap_pooled_connection_handler();

    bool handle_empty = true;
    Sequence s;
    EXPECT_CALL(handle_mock, empty()).WillRepeatedly(Invoke([&]{ return handle_empty;}));
    EXPECT_CALL(provider_mock, async_get_connection(_))
        .InSequence(s)
        .WillOnce(InvokeArgument<0>(error_code{}, make_connection()));
    EXPECT_CALL(handle_mock, reset(_)).InSequence(s).WillOnce(Invoke([&](auto){ handle_empty = false;}));
    EXPECT_CALL(io.stream_service_, create()).InSequence(s).WillOnce(ReturnRef(stream));
    EXPECT_CALL(native_handle, PQsocket()).InSequence(s).WillOnce(Return(42));
    EXPECT_CALL(stream, assign(42)).InSequence(s);

    EXPECT_CALL(callback_mock, call(ozo::error_code{}, _))
        .InSequence(s)
        .WillOnce(Return());

    EXPECT_CALL(stream, release()).InSequence(s);
    EXPECT_CALL(native_handle, PQstatus())
        .InSequence(s)
        .WillOnce(Return(CONNECTION_OK));
    EXPECT_CALL(native_handle, PQtransactionStatus())
        .InSequence(s)
        .WillOnce(Return(PQTRANS_IDLE));

    h({}, connection_pool::handle{&handle_mock});
}

TEST_F(pooled_connection_wrapper, should_invoke_callback_with_error_and_provided_connection_if_async_get_connection_fails) {
    auto h = wrap_pooled_connection_handler();

    bool handle_empty = true;
    Sequence s;
    EXPECT_CALL(handle_mock, empty()).WillRepeatedly(Invoke([&]{ return handle_empty;}));
    EXPECT_CALL(provider_mock, async_get_connection(_))
        .InSequence(s)
        .WillOnce(InvokeArgument<0>(error::error, make_connection()));
    EXPECT_CALL(handle_mock, reset(_)).InSequence(s).WillOnce(Invoke([&](auto){ handle_empty = false;}));
    EXPECT_CALL(io.stream_service_, create()).InSequence(s).WillOnce(ReturnRef(stream));
    EXPECT_CALL(native_handle, PQsocket()).InSequence(s).WillOnce(Return(42));
    EXPECT_CALL(stream, assign(42)).InSequence(s);

    EXPECT_CALL(callback_mock, call(Eq(error::error), _))
        .InSequence(s)
        .WillOnce(Return());

    EXPECT_CALL(stream, release()).InSequence(s);
    EXPECT_CALL(native_handle, PQstatus())
        .InSequence(s)
        .WillOnce(Return(CONNECTION_OK));
    EXPECT_CALL(native_handle, PQtransactionStatus())
        .InSequence(s)
        .WillOnce(Return(PQTRANS_IDLE));

    h({}, connection_pool::handle{&handle_mock});
}

TEST_F(pooled_connection_wrapper, should_invoke_callback_with_null_pointer_if_async_get_connection_provides_null_pointer) {
    auto h = wrap_pooled_connection_handler();

    bool handle_empty = true;
    Sequence s;
    EXPECT_CALL(handle_mock, empty()).WillRepeatedly(Invoke([&]{ return handle_empty;}));
    EXPECT_CALL(provider_mock, async_get_connection(_))
        .InSequence(s)
        .WillOnce(InvokeArgument<0>(error::error, nullptr));

    EXPECT_CALL(callback_mock, call(Eq(error::error), _))
        .InSequence(s)
        .WillOnce(Return());

    h({}, connection_pool::handle{&handle_mock});
}

} // namespace
