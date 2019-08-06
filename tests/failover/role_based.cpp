#include <ozo/failover/role_based.h>

#include "../test_error.h"

#include <boost/hana/equal.hpp>
#include <boost/hana/not_equal.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace {

using namespace testing;
using time_point = ozo::time_traits::time_point;
using duration = ozo::time_traits::duration;
using namespace std::chrono_literals;
namespace hana = boost::hana;

struct connection_mock {
    MOCK_CONST_METHOD0(close_connection, void());
    friend void close_connection(connection_mock* self) {
        if(!self) {
            throw std::invalid_argument("self should not be null");
        }
        self->close_connection();
    };
};

static constexpr ozo::failover::role<class test_role_tag> test_role;

struct role_based_connection_source_mock {
    MOCK_CONST_METHOD1(call, void(std::function<void(ozo::error_code, connection_mock*)>));
    MOCK_CONST_METHOD1(rebind_role, void(std::decay_t<decltype(ozo::failover::master)>));
    MOCK_CONST_METHOD1(rebind_role, void(std::decay_t<decltype(ozo::failover::replica)>));
    MOCK_CONST_METHOD1(rebind_role, void(std::decay_t<decltype(test_role)>));
    MOCK_CONST_METHOD0(move, void());
};

template <typename Role>
struct role_based_connection_source {
    using connection_type = connection_mock*;

    template <typename OtherRole>
    constexpr decltype(auto) rebind_role(OtherRole r) const & {
        mock_->rebind_role(r);
        return role_based_connection_source<OtherRole>{this->mock_};
    }

    template <typename OtherRole>
    constexpr decltype(auto) rebind_role(OtherRole r) && {
        mock_->move();
        mock_->rebind_role(r);
        return role_based_connection_source<OtherRole>{this->mock_};
    }

    template <typename IoContext, typename TimeConstraint, typename Handler>
    void operator() (IoContext&, TimeConstraint, Handler&& h) const {
        mock_->call(std::forward<Handler>(h));
    }

    role_based_connection_source_mock* mock_ = nullptr;
};

struct unsupported_role_connection_source {
    using connection_type = connection_mock*;

    template <typename IoContext, typename TimeConstraint, typename Handler>
    void operator() (IoContext&, TimeConstraint, Handler&&) const {
    }
};

} // namespace

namespace ozo {
// Some cheats about connection_mock which is not a connection
// at all.
template <>
struct is_connection<connection_mock*> : std::true_type {};

template <>
struct is_nullable<connection_mock*> : std::true_type {};

template <>
struct is_connection<::testing::StrictMock<connection_mock>*> : std::true_type {};

template <>
struct is_nullable<::testing::StrictMock<connection_mock>*> : std::true_type {};

} // namespace

namespace {

TEST(role_based_connection_provider__is_supported, should_return_true_for_connection_source_which_rebinds_for_role) {
    using provider = ozo::failover::role_based_connection_provider<role_based_connection_source<class dummy>>;
    EXPECT_TRUE(provider::is_supported(ozo::failover::master));
}

TEST(role_based_connection_provider__is_supported, should_return_false_for_connection_source_which_does_not_rebind_for_role) {
    using provider = ozo::failover::role_based_connection_provider<unsupported_role_connection_source>;
    EXPECT_FALSE(provider::is_supported(ozo::failover::master));
}

TEST(role_based_connection_provider__rebind, should_call_source_rebind_and_return_new_provider_for_role) {
    role_based_connection_source_mock source;
    boost::asio::io_service io;
    auto provider = ozo::failover::role_based_connection_provider{
                    role_based_connection_source<class dummy> {std::addressof(source)},
                    io
                };
    EXPECT_CALL(source, rebind_role(An<std::decay_t<decltype(ozo::failover::master)>>()));
    auto new_provider = provider.rebind_role(ozo::failover::master);
    using new_provider_type = ozo::failover::role_based_connection_provider<
                    role_based_connection_source<std::decay_t<decltype(ozo::failover::master)>>
                >;
    static_assert(std::is_same_v<decltype(new_provider), new_provider_type>,
        "rebind_role() should return provider specialized with a new source type"
    );
}

TEST(role_based_connection_provider__rebind, should_move_source_call_source_rebind_and_return_new_provider_for_role) {
    role_based_connection_source_mock source;
    boost::asio::io_service io;
    auto provider = ozo::failover::role_based_connection_provider{
                    role_based_connection_source<class dummy> {std::addressof(source)},
                    io
                };
    EXPECT_CALL(source, move());
    EXPECT_CALL(source, rebind_role(An<std::decay_t<decltype(ozo::failover::master)>>()));
    auto new_provider = std::move(provider).rebind_role(ozo::failover::master);
    using new_provider_type = ozo::failover::role_based_connection_provider<
                    role_based_connection_source<std::decay_t<decltype(ozo::failover::master)>>
                >;
    static_assert(std::is_same_v<decltype(new_provider), new_provider_type>,
        "rebind_role() should return provider specialized with a new source type"
    );
}

using namespace std::literals;

struct role_based_try__get_next_try : Test {
    StrictMock<connection_mock> conn;
    role_based_connection_source_mock source;
    boost::asio::io_service io;

    auto provider() {
        return ozo::failover::role_based_connection_provider{
            role_based_connection_source<class dummy>{std::addressof(source)},
            io
        };
    }

    struct handler_mock {
        MOCK_METHOD2(call, void(ozo::error_code, connection_mock*));
        template <typename Fallback>
        void operator() (ozo::error_code ec, connection_mock* conn, const Fallback&) {
            call(ec, conn);
        }
    };

    auto ctx() {
        return ozo::failover::basic_context(provider(), ozo::none);
    }

    connection_mock* connection() { return std::addressof(conn);}

    static constexpr connection_mock* null_conn = nullptr;

    StrictMock<handler_mock> handler;
};

} // namespace

namespace ozo::failover {
template <>
struct can_recover_impl<std::decay_t<decltype(::test_role)>> {
    static auto apply(decltype(::test_role), const error_code& ec) {
        return ozo::tests::error::error == ec;
    }
};
} // namespace ozo::failover

namespace {

TEST_F(role_based_try__get_next_try, should_return_next_try_for_matching_error) {
    using op = ozo::failover::role_based_options;
    auto role_based_try = ozo::failover::role_based_try(
        ozo::make_options(
            op::roles=hana::make_tuple(test_role, test_role)
        ), ctx()
    );
    EXPECT_FALSE(ozo::is_null(role_based_try.get_next_try(ozo::tests::error::error, null_conn)));
}

TEST_F(role_based_try__get_next_try, should_return_null_try_for_non_matching_error) {
    using op = ozo::failover::role_based_options;
    auto role_based_try = ozo::failover::role_based_try(
        ozo::make_options(
            op::roles=hana::make_tuple(test_role, test_role)
        ), ctx()
    );
    EXPECT_TRUE(ozo::is_null(role_based_try.get_next_try(ozo::tests::error::another_error, null_conn)));
}

TEST_F(role_based_try__get_next_try, should_return_null_try_for_no_roles_left) {
    using op = ozo::failover::role_based_options;
    auto role_based_try = ozo::failover::role_based_try(
        ozo::make_options(
            op::roles=hana::make_tuple(test_role)
        ), ctx()
    );
    EXPECT_TRUE(ozo::is_null(role_based_try.get_next_try(ozo::tests::error::error, null_conn)));
}

TEST_F(role_based_try__get_next_try, should_call_on_fallback_handler_for_matching_error_and_fallback) {
    using op = ozo::failover::role_based_options;
    auto role_based_try = ozo::failover::role_based_try(
        ozo::make_options(
            op::roles=hana::make_tuple(test_role, test_role),
            op::on_fallback=std::ref(handler)
        ), ctx()
    );
    EXPECT_CALL(handler, call(ozo::error_code{ozo::tests::error::error}, null_conn));
    role_based_try.get_next_try(ozo::tests::error::error, null_conn);
}

TEST_F(role_based_try__get_next_try, should_not_call_on_fallback_handler_for_non_matching_error) {
    using op = ozo::failover::role_based_options;
    auto role_based_try = ozo::failover::role_based_try(
        ozo::make_options(
            op::roles=hana::make_tuple(test_role, test_role),
            op::on_fallback=std::ref(handler)
        ), ctx()
    );
    role_based_try.get_next_try(ozo::tests::error::another_error, null_conn);
}

TEST_F(role_based_try__get_next_try, should_close_connection_on_retry_if_option_is_omitted) {
    EXPECT_CALL(conn, close_connection());
    using op = ozo::failover::role_based_options;
    auto role_based_try = ozo::failover::role_based_try(
        ozo::make_options(
            op::roles=hana::make_tuple(test_role, test_role)
        ), ctx()
    );
    role_based_try.get_next_try(ozo::tests::error::error, connection());
}

TEST_F(role_based_try__get_next_try, should_close_connection_on_retry_if_option_is_true) {
    EXPECT_CALL(conn, close_connection());
    using op = ozo::failover::role_based_options;
    auto role_based_try = ozo::failover::role_based_try(
        ozo::make_options(
            op::roles=hana::make_tuple(test_role, test_role),
            op::close_connection=true
        ), ctx()
    );
    role_based_try.get_next_try(ozo::tests::error::error, connection());
}

TEST_F(role_based_try__get_next_try, should_not_close_connection_on_retry_if_option_is_false) {
    using op = ozo::failover::role_based_options;
    auto role_based_try = ozo::failover::role_based_try(
        ozo::make_options(
            op::roles=hana::make_tuple(test_role, test_role),
            op::close_connection=false
        ), ctx()
    );
    role_based_try.get_next_try(ozo::tests::error::error, connection());
}

TEST_F(role_based_try__get_next_try, should_close_connection_on_no_retry_if_option_is_omitted) {
    EXPECT_CALL(conn, close_connection());
    using op = ozo::failover::role_based_options;
    auto role_based_try = ozo::failover::role_based_try(
        ozo::make_options(
            op::roles=hana::make_tuple(test_role)
        ), ctx()
    );
    role_based_try.get_next_try(ozo::tests::error::error, connection());
}

TEST_F(role_based_try__get_next_try, should_close_connection_on_no_retry_if_option_is_true) {
    EXPECT_CALL(conn, close_connection());
    using op = ozo::failover::role_based_options;
    auto role_based_try = ozo::failover::role_based_try(
        ozo::make_options(
            op::roles=hana::make_tuple(test_role),
            op::close_connection=true
        ), ctx()
    );
    role_based_try.get_next_try(ozo::tests::error::error, connection());
}

TEST_F(role_based_try__get_next_try, should_not_close_connection_on_no_retry_if_option_is_false) {
    using op = ozo::failover::role_based_options;
    auto role_based_try = ozo::failover::role_based_try(
        ozo::make_options(
            op::roles=hana::make_tuple(test_role),
            op::close_connection=false
        ), ctx()
    );
    role_based_try.get_next_try(ozo::tests::error::error, connection());
}


struct role_based_try__get_context : Test {
    role_based_connection_source_mock source;
    boost::asio::io_service io;

    auto provider() {
        return ozo::failover::role_based_connection_provider{
            role_based_connection_source<class dummy>{std::addressof(source)},
            io
        };
    }
    auto ctx() {
        return ozo::failover::basic_context(provider(), ozo::none);
    }

    static constexpr connection_mock* null_conn = nullptr;
};

TEST_F(role_based_try__get_context, should_return_rebound_provider_form_context) {

    using op = ozo::failover::role_based_options;
    auto role_based_try = ozo::failover::role_based_try(
        ozo::make_options(
            op::roles=hana::make_tuple(test_role, test_role)
        ), ctx()
    );
    role_based_try.get_next_try(ozo::tests::error::error, null_conn);
    EXPECT_CALL(source, rebind_role(An<std::decay_t<decltype(test_role)>>()));
    auto new_provider = role_based_try.get_context()[hana::size_c<0>];

    using new_provider_type = ozo::failover::role_based_connection_provider<
                    role_based_connection_source<std::decay_t<decltype(test_role)>>>;
    static_assert(std::is_same_v<decltype(new_provider), new_provider_type>,
        "rebind_role() should return provider specialized with a new source type"
    );
}

TEST_F(role_based_try__get_context, should_return_calculated_time_out_as_divided_for_two_tries_form_context) {
    using op = ozo::failover::role_based_options;
    auto role_based_try = ozo::failover::role_based_try(
        ozo::make_options(
            op::roles=hana::make_tuple(test_role, test_role)
        ),
        ozo::failover::basic_context(provider(), 4s)
    );
    EXPECT_CALL(source, rebind_role(An<std::decay_t<decltype(test_role)>>()));
    EXPECT_EQ(role_based_try.get_context()[hana::size_c<1>], 2s);
}

TEST_F(role_based_try__get_context, should_return_whole_time_out_for_two_single_try_form_context) {
    using op = ozo::failover::role_based_options;
    auto role_based_try = ozo::failover::role_based_try(
        ozo::make_options(
            op::roles=hana::make_tuple(test_role)
        ),
        ozo::failover::basic_context(provider(), 4s)
    );
    EXPECT_CALL(source, rebind_role(An<std::decay_t<decltype(test_role)>>()));
    EXPECT_EQ(role_based_try.get_context()[hana::size_c<1>], 4s);
}

} // namespace
