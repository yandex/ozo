#include <ozo/connection_info.h>
#include <ozo/connection_pool.h>

#include <GUnit/GTest.h>

#include "connection_mock.h"
#include "test_error.h"

GTEST("ozo::make_connection_pool") {
    SHOULD("not throw") {
        boost::asio::io_context io;
        const ozo::connection_info<> conn_info(io, "conn info string");
        const ozo::connection_pool_config config;
        EXPECT_NO_THROW(ozo::make_connection_pool(conn_info, config));
    }
}

namespace ozo::testing {

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
    MOCK_METHOD1(async_get_connection, void(handler_type));

    virtual ~connection_provider_mock() = default;
};

struct connection_provider {
    connection_provider_mock* mock_ = nullptr;

    using connectable_type = std::shared_ptr<connection<>>;

    template <typename Handler>
    friend void async_get_connection(connection_provider self, Handler&& h) {
        self.mock_->async_get_connection(std::forward<Handler>(h));
    }
};
} // namespace ozo::testing

namespace ozo::impl {
template <>
struct get_connection_pool<ozo::testing::connection_provider> {
    using type = ozo::testing::connection_pool;
};
} // namespace ozo::impl



GTEST("ozo::impl::pooled_connection") {
    using namespace ozo::testing;
    using namespace ::testing;

    using pooled_connection = ozo::impl::pooled_connection<connection_provider>;

    SHOULD("call handle.waste() on destruction if handle is not empty and connection is bad") {
        StrictMock<connection_gmock> connection_mock{};
        StrictMock<pool_handle_mock> handle_mock{};

        auto conn = connection<>{};
        conn.mock_ = &connection_mock;
        conn.handle_ = std::make_unique<native_handle>(native_handle::bad);

        EXPECT_CALL(handle_mock, value()).WillRepeatedly(ReturnRef(conn));
        EXPECT_CALL(handle_mock, empty()).WillRepeatedly(Return(false));
        EXPECT_CALL(handle_mock, waste()).WillOnce(Return());

        {
            pooled_connection p(connection_pool::handle{&handle_mock});
        }
    }

    SHOULD("call nothing on destruction if handle is not empty and connection is good") {
        StrictMock<connection_gmock> connection_mock{};
        StrictMock<pool_handle_mock> handle_mock{};

        auto conn = connection<>{};
        conn.mock_ = &connection_mock;
        conn.handle_ = std::make_unique<native_handle>(native_handle::good);

        EXPECT_CALL(handle_mock, value()).WillRepeatedly(ReturnRef(conn));
        EXPECT_CALL(handle_mock, empty()).WillRepeatedly(Return(false));

        {
            pooled_connection p(connection_pool::handle{&handle_mock});
        }
    }

    SHOULD("call nothing on destruction if handle is empty") {
        StrictMock<pool_handle_mock> handle_mock{};

        EXPECT_CALL(handle_mock, empty()).WillRepeatedly(Return(true));

        {
            pooled_connection p(connection_pool::handle{&handle_mock});
        }
    }

    SHOULD("call handle.reset() on reset()") {
        StrictMock<pool_handle_mock> handle_mock{};

        EXPECT_CALL(handle_mock, empty()).WillRepeatedly(Return(true));
        {
            pooled_connection p(connection_pool::handle{&handle_mock});
            EXPECT_CALL(handle_mock, reset(_)).WillOnce(Return());
            p.reset(connection<>{});
        }
    }
}

GTEST("ozo::impl::pooled_connection_wrapper") {
    using namespace ozo::testing;
    using namespace ::testing;
    using ozo::error_code;

    using pooled_connection_ptr = ozo::impl::pooled_connection_ptr<connection_provider>;

    SHOULD("invoke handler with error if error passed") {
        StrictMock<connection_provider_mock> provider_mock;
        StrictMock<callback_gmock<pooled_connection_ptr>> callback_mock;

        auto h = ozo::impl::wrap_pooled_connection_handler(
                connection_provider{&provider_mock},
                wrap(callback_mock)
            );

        EXPECT_CALL(callback_mock, call(error_code(error::error), _)).WillOnce(Return());

        h(error::error, connection_pool::handle{});
    }

    SHOULD("invoke handler if passed connection is good and handle is not empty") {
        StrictMock<connection_provider_mock> provider_mock;
        StrictMock<callback_gmock<pooled_connection_ptr>> callback_mock;
        StrictMock<connection_gmock> connection_mock{};
        StrictMock<pool_handle_mock> handle_mock{};

        auto conn = connection<>{};
        conn.mock_ = &connection_mock;
        conn.handle_ = std::make_unique<native_handle>(native_handle::good);

        auto h = ozo::impl::wrap_pooled_connection_handler(
                connection_provider{&provider_mock},
                wrap(callback_mock)
            );

        EXPECT_CALL(handle_mock, value()).WillRepeatedly(ReturnRef(conn));
        EXPECT_CALL(handle_mock, empty()).WillRepeatedly(Return(false));

        EXPECT_CALL(callback_mock, call(_, _)).WillOnce(Return());

        h({}, connection_pool::handle{&handle_mock});
    }

    SHOULD("call async_get_connection(provider, handler) and invoke handler if passed connection is bad and handle is not empty") {
        StrictMock<connection_provider_mock> provider_mock;
        StrictMock<callback_gmock<pooled_connection_ptr>> callback_mock;
        StrictMock<connection_gmock> connection_mock{};
        StrictMock<pool_handle_mock> handle_mock{};

        auto bad_conn = connection<>{};
        bad_conn.mock_ = &connection_mock;
        bad_conn.handle_ = std::make_unique<native_handle>(native_handle::bad);

        auto good_conn = std::make_shared<connection<>>();
        good_conn->handle_ = std::make_unique<native_handle>(native_handle::good);

        auto h = ozo::impl::wrap_pooled_connection_handler(
                connection_provider{&provider_mock},
                wrap(callback_mock)
            );

        connection<> conn = std::move(bad_conn);

        EXPECT_CALL(handle_mock, value()).WillRepeatedly(ReturnRef(conn));
        EXPECT_CALL(handle_mock, empty()).WillRepeatedly(Return(false));

        EXPECT_CALL(provider_mock, async_get_connection(_))
            .WillOnce(InvokeArgument<0>(error_code{}, good_conn));

        EXPECT_CALL(handle_mock, reset(_))
            .WillOnce(Invoke([&](connection<>& c){
                conn = std::move(c);
            }));

        EXPECT_CALL(callback_mock, call(_, _)).WillOnce(Return());

        h({}, connection_pool::handle{&handle_mock});
    }

    SHOULD("call async_get_connection(provider, handler) and invoke handler if handle is empty") {
        StrictMock<connection_provider_mock> provider_mock;
        StrictMock<callback_gmock<pooled_connection_ptr>> callback_mock;
        StrictMock<pool_handle_mock> handle_mock{};

        auto h = ozo::impl::wrap_pooled_connection_handler(
                connection_provider{&provider_mock},
                wrap(callback_mock)
            );

        EXPECT_CALL(handle_mock, empty()).WillRepeatedly(Return(true));

        EXPECT_CALL(provider_mock, async_get_connection(_))
            .WillOnce(InvokeArgument<0>(
                error_code{}, std::make_shared<connection<>>())
            );

        EXPECT_CALL(handle_mock, reset(_)).WillOnce(Return());
        EXPECT_CALL(callback_mock, call(_, _)).WillOnce(Return());

        h({}, connection_pool::handle{&handle_mock});
    }

    SHOULD("invoke callback with an error if async_get_connection(provider, handler) provides the error") {
        StrictMock<connection_provider_mock> provider_mock;
        StrictMock<callback_gmock<pooled_connection_ptr>> callback_mock;
        StrictMock<pool_handle_mock> handle_mock{};

        auto h = ozo::impl::wrap_pooled_connection_handler(
                connection_provider{&provider_mock},
                wrap(callback_mock)
            );

        EXPECT_CALL(handle_mock, empty()).WillRepeatedly(Return(true));

        EXPECT_CALL(provider_mock, async_get_connection(_))
            .WillOnce(InvokeArgument<0>(
                error::error, std::make_shared<connection<>>())
            );

        EXPECT_CALL(callback_mock, call(error_code{error::error}, _)).WillOnce(Return());

        h({}, connection_pool::handle{&handle_mock});
    }
}
