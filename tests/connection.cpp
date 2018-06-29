#include <ozo/connection.h>

#include <boost/make_shared.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>
#include <boost/fusion/adapted/struct/define_struct.hpp>
#include <boost/fusion/include/define_struct.hpp>

#include <boost/hana/adapt_adt.hpp>

#include "test_asio.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace {

namespace hana = ::boost::hana;

enum class native_handle { bad, good };

inline bool connection_status_bad(const native_handle* h) {
    return *h == native_handle::bad;
}

using ozo::empty_oid_map;

struct socket_mock {
    socket_mock& get_io_service() { return *this;}
    template <typename Handler>
    void post(Handler&& h) {
        ozo::testing::asio_post(std::forward<Handler>(h));
    }
    template <typename Handler>
    void dispatch(Handler&& h) {
        ozo::testing::asio_post(std::forward<Handler>(h));
    }
};

template <typename OidMap = empty_oid_map>
struct connection {
    using handle_type = std::unique_ptr<native_handle>;

    handle_type handle_ = std::make_unique<native_handle>();
    socket_mock socket_;
    OidMap oid_map_;
    std::string error_context_;

    friend OidMap& get_connection_oid_map(connection& conn) {
        return conn.oid_map_;
    }
    friend const OidMap& get_connection_oid_map(const connection& conn) {
        return conn.oid_map_;
    }
    friend socket_mock& get_connection_socket(connection& conn) {
        return conn.socket_;
    }
    friend const socket_mock& get_connection_socket(const connection& conn) {
        return conn.socket_;
    }
    friend handle_type& get_connection_handle(connection& conn) {
        return conn.handle_;
    }
    friend const handle_type& get_connection_handle(const connection& conn) {
        return conn.handle_;
    }
    friend std::string& get_connection_error_context(connection& conn) {
        return conn.error_context_;
    }
    friend const std::string& get_connection_error_context(const connection& conn) {
        return conn.error_context_;
    }
};

template <typename ...Ts>
using connection_ptr = std::shared_ptr<connection<Ts...>>;

static_assert(ozo::Connection<connection<>>,
    "connection does not meet Connection requirements");
static_assert(ozo::ConnectionWrapper<connection_ptr<>>,
    "connection_ptr does not meet ConnectionWrapper requirements");
static_assert(ozo::Connectable<connection<>>,
    "connection does not meet Connectable requirements");
static_assert(ozo::Connectable<connection_ptr<>>,
    "connection_ptr does not meet Connectable requirements");

static_assert(!ozo::Connection<int>,
    "int meets Connection requirements unexpectedly");
static_assert(!ozo::ConnectionWrapper<int>,
    "int meets ConnectionWrapper requirements unexpectedly");
static_assert(!ozo::Connectable<int>,
    "int meets Connectable requirements unexpectedly");

TEST(connection_good, should_return_false_for_object_with_bad_handle) {
    auto conn = std::make_shared<connection<>>();
    *(conn->handle_) = native_handle::bad;
    EXPECT_FALSE(ozo::connection_good(conn));
}

TEST(connection_good, should_return_false_for_object_with_nullptr) {
    connection_ptr<> conn;
    EXPECT_FALSE(ozo::connection_good(conn));
}

TEST(connection_good, should_return_true_for_object_with_good_handle) {
    auto conn = std::make_shared<connection<>>();
    *(conn->handle_) = native_handle::good;
    EXPECT_TRUE(ozo::connection_good(conn));
}

TEST(connection_bad, should_return_true_for_object_with_bad_handle) {
    auto conn = std::make_shared<connection<>>();
    *(conn->handle_) = native_handle::bad;
    EXPECT_TRUE(ozo::connection_bad(conn));
}

TEST(connection_bad, should_return_true_for_object_with_nullptr) {
    connection_ptr<> conn;
    EXPECT_TRUE(ozo::connection_bad(conn));
}

TEST(connection_bad, should_return_false_for_object_with_good_handle) {
    auto conn = std::make_shared<connection<>>();
    *(conn->handle_) = native_handle::good;
    EXPECT_FALSE(ozo::connection_bad(conn));
}

TEST(unwrap_connection, should_return_connection_reference_for_connection_wrapper) {
    auto conn = std::make_shared<connection<>>();

    EXPECT_EQ(
        std::addressof(ozo::unwrap_connection(conn)),
        conn.get()
    );
}

TEST(unwrap_connection, should_return_argument_reference_for_connection) {
    auto conn = connection<>();

    EXPECT_EQ(
        std::addressof(ozo::unwrap_connection(conn)),
        std::addressof(conn)
    );
}

TEST(get_error_context, should_returns_reference_to_error_context) {
    auto conn = std::make_shared<connection<>>();

    EXPECT_EQ(
        std::addressof(ozo::get_error_context(conn)),
        std::addressof(conn->error_context_)
    );
}

TEST(set_error_context, should_set_error_context) {
    auto conn = std::make_shared<connection<>>();
    ozo::set_error_context(conn, "brand new super context");

    EXPECT_EQ(conn->error_context_, "brand new super context");
}

TEST(reset_error_context, should_resets_error_context) {
    auto conn = std::make_shared<connection<>>();
    conn->error_context_ = "brand new super context";
    ozo::reset_error_context(conn);
    EXPECT_TRUE(conn->error_context_.empty());
}

using ozo::error_code;
using namespace ::testing;

TEST(get_connection, should_pass_through_the_connection_to_handler) {
    auto conn = std::make_shared<connection<>>();
    using callback_mock = ozo::testing::callback_gmock<decltype(conn)>;
    using ozo::testing::wrap;
    StrictMock<callback_mock> cb_mock{};
    EXPECT_CALL(cb_mock, context_preserved()).WillOnce(Return());
    EXPECT_CALL(cb_mock, call(error_code{}, conn)).WillOnce(Return());
    ozo::get_connection(conn, wrap(cb_mock));
}

TEST(get_connection, should_reset_connection_error_context) {
    auto conn = std::make_shared<connection<>>();
    conn->error_context_ = "some context here";
    ozo::get_connection(conn, [](error_code, auto conn) {
        EXPECT_TRUE(conn->error_context_.empty());
    });
}

} //namespace
