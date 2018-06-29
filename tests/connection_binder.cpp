#include <ozo/impl/async_connect.h>
#include "test_error.h"
#include "connection_mock.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace ozo::testing {
    struct custom_type {};
} // namespace ozo::testing

OZO_PG_DEFINE_CUSTOM_TYPE(ozo::testing::custom_type, "custom_type", dynamic_size)

namespace {

using ozo::empty_oid_map;

struct connection_mock {
    MOCK_METHOD0(request_oid_map, void());
};

using callback_mock = ozo::testing::callback_mock<>;
using ozo::testing::wrap;

template <typename OidMap = empty_oid_map>
struct connection_wrapper {
    connection_mock& mock_;
    OidMap oid_map_;

    friend OidMap& get_oid_map(connection_wrapper& conn) {
        return conn.oid_map_;
    }

    template <typename Handler>
    friend void request_oid_map(connection_wrapper c, Handler&&) {
        c.mock_.request_oid_map();
    }
};


using namespace testing;
using ozo::error_code;

struct connection_binder : Test {
    StrictMock<connection_mock> connection{};
    template <typename OidMap>
    auto make_connection(OidMap oid_map) {
        return ::connection_wrapper<OidMap>{connection, oid_map};
    }

    template <typename Conn>
    StrictMock<ozo::testing::callback_gmock<std::decay_t<Conn>>>
    make_callback(Conn&&) { return {}; }
};

TEST_F(connection_binder, should_request_for_oid_when_oid_map_is_not_empty) {
    auto conn = make_connection(ozo::register_types<ozo::testing::custom_type>());
    auto callback = make_callback(conn);

    EXPECT_CALL(connection, request_oid_map()).WillOnce(Return());

    ozo::impl::bind_connection_handler(wrap(callback), std::move(conn))(error_code{});
}

TEST_F(connection_binder, should_not_request_for_oid_when_oid_map_is_not_empty_but_error_occured) {
    auto conn = make_connection(ozo::register_types<ozo::testing::custom_type>());
    auto callback = make_callback(conn);

    EXPECT_CALL(callback, call(error_code{ozo::testing::error::error}, _))
        .WillOnce(Return());

    ozo::impl::bind_connection_handler(wrap(callback), std::move(conn))(ozo::testing::error::error);
}

TEST_F(connection_binder, should_not_request_for_oid_when_oid_map_ist_empty) {
    auto conn = make_connection(ozo::register_types<>());
    auto callback = make_callback(conn);

    EXPECT_CALL(callback, call(error_code{}, _)).WillOnce(Return());

    ozo::impl::bind_connection_handler(wrap(callback), std::move(conn))(error_code{});
}

} // namespace
