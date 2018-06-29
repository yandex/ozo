#include <ozo/impl/async_connect.h>
#include "test_error.h"
#include "connection_mock.h"
#include <GUnit/GTest.h>

namespace ozo::testing {
    struct custom_type {};
} // namespace ozo::testing

OZO_PG_DEFINE_CUSTOM_TYPE(ozo::testing::custom_type, "custom_type", dynamic_size)

namespace {

using ozo::empty_oid_map;
using ::testing::StrictMock;

struct connection_mock {
    MOCK_METHOD0(request_oid_map, void());
};

using callback_mock = ozo::testing::callback_mock<>;
using ozo::testing::wrap;

template <typename OidMap = empty_oid_map>
struct connection {
    connection_mock& mock_;
    OidMap oid_map_;

    friend OidMap& get_oid_map(connection& conn) {
        return conn.oid_map_;
    }

    template <typename Handler>
    friend void request_oid_map(connection c, Handler&&) {
        c.mock_.request_oid_map();
    }
};

template <typename OidMap>
auto make_connection(connection_mock& mock, OidMap oid_map) {
    return connection<OidMap>{mock, oid_map};
}

GTEST("ozo::detail::connection_binder") {
    using namespace testing;
    using ozo::error_code;

    SHOULD("request for oid when oid map is not empty") {
        StrictMock<connection_mock> connection{};
        auto conn = make_connection(connection, ozo::register_types<ozo::testing::custom_type>());
        StrictMock<ozo::testing::callback_gmock<std::decay_t<decltype(conn)>>> callback{};

        EXPECT_CALL(connection, request_oid_map()).WillOnce(Return());
        ozo::impl::bind_connection_handler(wrap(callback), std::move(conn))(error_code{});
    }

    SHOULD("request for oid when there is an error adn oid map is not empty") {
        StrictMock<connection_mock> connection{};
        auto conn = make_connection(connection, ozo::register_types<ozo::testing::custom_type>());
        StrictMock<ozo::testing::callback_gmock<std::decay_t<decltype(conn)>>> callback{};

        EXPECT_CALL(callback, call(error_code{ozo::testing::error::error}, _))
            .WillOnce(Return());
        ozo::impl::bind_connection_handler(wrap(callback), std::move(conn))(ozo::testing::error::error);
    }

    SHOULD("not request for oid and call handler when oid map is empty") {
        StrictMock<connection_mock> connection{};
        auto conn = make_connection(connection, ozo::register_types<>());
        StrictMock<ozo::testing::callback_gmock<std::decay_t<decltype(conn)>>> callback{};

        EXPECT_CALL(callback, call(error_code{}, _)).WillOnce(Return());
        ozo::impl::bind_connection_handler(wrap(callback), std::move(conn))(error_code{});
    }
}

} // namespace
