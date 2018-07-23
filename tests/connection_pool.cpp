#include "connection_mock.h"
#include "test_error.h"

#include <ozo/connection_info.h>
#include <ozo/connection_pool.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace {

TEST(make_connection_pool, sould_not_throw) {
    boost::asio::io_context io;
    const ozo::connection_info<> conn_info("conn info string");
    const ozo::connection_pool_config config;
    EXPECT_NO_THROW(ozo::make_connection_pool(conn_info, config));
}

} //namespace

namespace ozo::tests {

struct pool_handle_mock {
    using value_type = connection<>;
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
        handle(const handle&) = delete;
        handle(handle&&) = default;
        handle& operator = (const handle& ) = delete;
        handle& operator = (handle&& ) = default;
        bool empty() const { return mock_->empty(); }
        void reset(value_type v) { mock_->reset(v); }
        void waste() { mock_->waste(); }
        value_type& operator * () { return mock_->value(); }
        const value_type& operator * () const { return mock_->value(); }
    };
};

struct connection_provider;
struct connection_provider_mock {
    using connectable_type = std::shared_ptr<connection<>>;
    using handler_type = std::function<void(error_code, connectable_type)>;
    MOCK_METHOD2(async_get_connection, void(io_context&, handler_type));

    virtual ~connection_provider_mock() = default;
};

struct connection_provider {
    connection_provider_mock* mock_ = nullptr;

    using connectable_type = std::shared_ptr<connection<>>;

    template <typename Handler>
    friend void async_get_connection(connection_provider self, io_context& io, Handler&& h) {
        self.mock_->async_get_connection(io, std::forward<Handler>(h));
    }
};
} // namespace ozo::tests

namespace ozo::impl {
template <>
struct get_connection_pool<ozo::tests::connection_provider> {
    using type = ozo::tests::connection_pool;
};
} // namespace ozo::impl

namespace {

using namespace ozo::tests;
using namespace testing;

struct pooled_connection : Test {
    StrictMock<connection_gmock> connection_mock{};
    StrictMock<pool_handle_mock> handle_mock{};

    using impl = ozo::impl::pooled_connection<connection_provider>;

    auto make_connection(native_handle h) {
        auto conn = connection<>{};
        conn.mock_ = &connection_mock;
        conn.handle_ = std::make_unique<native_handle>(h);
        return conn;
    }
};

TEST_F(pooled_connection, should_call_handle_waste_on_destruction_if_handle_is_not_empty_and_connection_is_bad) {
    auto conn = make_connection(native_handle::bad);

    EXPECT_CALL(handle_mock, value()).WillRepeatedly(ReturnRef(conn));
    EXPECT_CALL(handle_mock, empty()).WillRepeatedly(Return(false));
    EXPECT_CALL(handle_mock, waste()).WillOnce(Return());

    {
        impl p(connection_pool::handle{&handle_mock});
    }
}

TEST_F(pooled_connection, should_call_nothing_on_destruction_if_handle_is_not_empty_and_connection_is_good) {
    auto conn = make_connection(native_handle::good);

    EXPECT_CALL(handle_mock, value()).WillRepeatedly(ReturnRef(conn));
    EXPECT_CALL(handle_mock, empty()).WillRepeatedly(Return(false));

    {
        impl p(connection_pool::handle{&handle_mock});
    }
}

TEST_F(pooled_connection, should_call_nothing_on_destruction_if_handle_is_empty) {
    EXPECT_CALL(handle_mock, empty()).WillRepeatedly(Return(true));

    {
        impl p(connection_pool::handle{&handle_mock});
    }
}

TEST_F(pooled_connection, should_call_handle_reset_on_reset) {
    EXPECT_CALL(handle_mock, empty()).WillRepeatedly(Return(true));
    {
        impl p(connection_pool::handle{&handle_mock});
        EXPECT_CALL(handle_mock, reset(_)).WillOnce(Return());
        p.reset(connection<>{});
    }
}

struct pooled_connection_wrapper : Test {
    using pooled_connection_ptr = ozo::impl::pooled_connection_ptr<connection_provider>;
    StrictMock<connection_provider_mock> provider_mock;
    StrictMock<callback_gmock<pooled_connection_ptr>> callback_mock;
    StrictMock<connection_gmock> connection_mock{};
    StrictMock<pool_handle_mock> handle_mock{};
    StrictMock<executor_gmock> executor_mock{};
    StrictMock<strand_executor_service_gmock> strand_mock{};
    io_context io{executor_mock, strand_mock};

    auto make_connection(native_handle h) {
        auto conn = connection<>{};
        conn.mock_ = &connection_mock;
        conn.handle_ = std::make_unique<native_handle>(h);
        return conn;
    }
};

using ozo::error_code;

TEST_F(pooled_connection_wrapper, should_invoke_handler_with_error_if_error_is_passed) {
    auto h = ozo::impl::wrap_pooled_connection_handler(
            io,
            connection_provider{&provider_mock},
            wrap(callback_mock)
        );

    EXPECT_CALL(callback_mock, call(error_code(error::error), _)).WillOnce(Return());

    h(error::error, connection_pool::handle{});
}

TEST_F(pooled_connection_wrapper, should_invoke_handler_if_passed_connection_is_good_and_handle_is_not_empty) {
    auto conn = make_connection(native_handle::good);

    auto h = ozo::impl::wrap_pooled_connection_handler(
            io,
            connection_provider{&provider_mock},
            wrap(callback_mock)
        );

    EXPECT_CALL(handle_mock, value()).WillRepeatedly(ReturnRef(conn));
    EXPECT_CALL(handle_mock, empty()).WillRepeatedly(Return(false));
    EXPECT_CALL(connection_mock, rebind_io_context()).WillRepeatedly(Return(ozo::error_code{}));

    EXPECT_CALL(callback_mock, call(_, _)).WillOnce(Return());

    h({}, connection_pool::handle{&handle_mock});
}

TEST_F(pooled_connection_wrapper, should_call_async_get_connection_and_invoke_handler_if_passed_connection_is_bad_and_handle_is_not_empty) {
    auto bad_conn = make_connection(native_handle::bad);

    auto good_conn = std::make_shared<connection<>>();
    good_conn->handle_ = std::make_unique<native_handle>(native_handle::good);

    auto h = ozo::impl::wrap_pooled_connection_handler(
            io,
            connection_provider{&provider_mock},
            wrap(callback_mock)
        );

    connection<> conn = std::move(bad_conn);

    EXPECT_CALL(handle_mock, value()).WillRepeatedly(ReturnRef(conn));
    EXPECT_CALL(handle_mock, empty()).WillRepeatedly(Return(false));

    EXPECT_CALL(provider_mock, async_get_connection(_, _))
        .WillOnce(InvokeArgument<1>(error_code{}, good_conn));

    EXPECT_CALL(handle_mock, reset(_))
        .WillOnce(Invoke([&](connection<>& c){
            conn = std::move(c);
        }));

    EXPECT_CALL(callback_mock, call(_, _)).WillOnce(Return());

    h({}, connection_pool::handle{&handle_mock});
}

TEST_F(pooled_connection_wrapper, should_call_async_get_connection_and_invoke_handler_if_handle_is_empty) {
    auto h = ozo::impl::wrap_pooled_connection_handler(
            io,
            connection_provider{&provider_mock},
            wrap(callback_mock)
        );

    EXPECT_CALL(handle_mock, empty()).WillRepeatedly(Return(true));

    EXPECT_CALL(provider_mock, async_get_connection(_, _))
        .WillOnce(InvokeArgument<1>(
            error_code{}, std::make_shared<connection<>>())
        );

    EXPECT_CALL(handle_mock, reset(_)).WillOnce(Return());
    EXPECT_CALL(callback_mock, call(_, _)).WillOnce(Return());

    h({}, connection_pool::handle{&handle_mock});
}

TEST_F(pooled_connection_wrapper, should_invoke_callback_with_error_if_async_get_connection_fails) {
    auto h = ozo::impl::wrap_pooled_connection_handler(
            io,
            connection_provider{&provider_mock},
            wrap(callback_mock)
        );

    EXPECT_CALL(handle_mock, empty()).WillRepeatedly(Return(true));

    EXPECT_CALL(provider_mock, async_get_connection(_, _))
        .WillOnce(InvokeArgument<1>(
            error::error, std::make_shared<connection<>>())
        );

    EXPECT_CALL(callback_mock, call(error_code{error::error}, _)).WillOnce(Return());

    h({}, connection_pool::handle{&handle_mock});
}

} // namespace
