#include "connection_mock.h"
#include "test_error.h"

#include <ozo/impl/async_connect.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace ozo::tests {

struct custom_type {};

} // namespace ozo::tests

OZO_PG_DEFINE_CUSTOM_TYPE(ozo::tests::custom_type, "custom_type", dynamic_size)

namespace {

using namespace testing;
using namespace ozo::tests;

using ozo::empty_oid_map;
using ozo::error_code;

struct connection_mock {
    MOCK_METHOD0(request_oid_map, void());
};

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

struct request_oid_map_handler : Test {
    StrictMock<connection_mock> connection{};

    template <typename OidMap>
    auto make_connection(OidMap oid_map) {
        return connection_wrapper<OidMap>{connection, oid_map};
    }

    template <typename Conn>
    auto make_callback(Conn&&) {
        return StrictMock<callback_gmock<std::decay_t<Conn>>> {};
    }
};

TEST_F(request_oid_map_handler, should_request_for_oid_when_oid_map_is_not_empty) {
    auto conn = make_connection(ozo::register_types<custom_type>());
    auto callback = make_callback(conn);

    EXPECT_CALL(connection, request_oid_map()).WillOnce(Return());

    ozo::impl::make_request_oid_map_handler(wrap(callback))(error_code{}, std::move(conn));
}

TEST_F(request_oid_map_handler, should_not_request_for_oid_when_oid_map_is_not_empty_but_error_occured) {
    auto conn = make_connection(ozo::register_types<custom_type>());
    auto callback = make_callback(conn);

    EXPECT_CALL(callback, call(error_code{error::error}, _))
        .WillOnce(Return());

    ozo::impl::make_request_oid_map_handler(wrap(callback))(error::error, std::move(conn));
}

TEST_F(request_oid_map_handler, should_not_request_for_oid_when_oid_map_ist_empty) {
    auto conn = make_connection(ozo::register_types<>());
    auto callback = make_callback(conn);

    EXPECT_CALL(callback, call(error_code{}, _)).WillOnce(Return());

    ozo::impl::make_request_oid_map_handler(wrap(callback))(error_code{}, std::move(conn));
}

} // namespace
