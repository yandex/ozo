#include <ozo/connection_info.h>
#include <ozo/query_builder.h>
#include <ozo/request.h>
#include <ozo/shortcuts.h>
#include <ozo/failover/role_based.h>

#include <boost/asio/spawn.hpp>
#include <boost/range/adaptor/transformed.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace {

namespace hana = boost::hana;
namespace failover = ozo::failover;
using op = failover::role_based_options;

using namespace testing;

struct callback_mock {
    template <typename Connection>
    void operator () (ozo::error_code ec, Connection&&) const {
        call(ec);
    }
    template <typename Connection, typename Fallback>
    void operator () (ozo::error_code ec, Connection&&, const Fallback&) const {
        call(ec);
    }
    MOCK_CONST_METHOD1(call, void(ozo::error_code));
};

TEST(role_based, should_return_success_for_invalid_connection_info_retried_with_valid_connection_info) {
    using namespace ozo::literals;
    using namespace hana::literals;

    ozo::io_context io;
    auto conn_info = failover::make_role_based_connection_source(
        failover::master=ozo::connection_info("invalid connection info"),
        failover::replica=ozo::connection_info(OZO_PG_TEST_CONNINFO)
    );
    StrictMock<callback_mock> callback;
    auto roles = failover::role_based(failover::master, failover::replica)
                        .set(op::on_fallback=std::cref(callback));

    InSequence s;
    EXPECT_CALL(callback, call(Eq(ozo::errc::connection_error)));
    EXPECT_CALL(callback, call(ozo::error_code{}));

    std::vector<int> res;
    ozo::request[roles](conn_info[io], "SELECT 1"_SQL + " + 1"_SQL, ozo::into(res),
            std::cref(callback));

    io.run();
}

TEST(role_based, should_not_try_next_role_and_return_error_for_sql_syntax_error) {
    using namespace ozo::literals;
    using namespace hana::literals;

    ozo::io_context io;
    auto conn_info = failover::make_role_based_connection_source(
        failover::master=ozo::connection_info(OZO_PG_TEST_CONNINFO),
        failover::replica=ozo::connection_info(OZO_PG_TEST_CONNINFO)
    );
    StrictMock<callback_mock> callback;
    auto roles = failover::role_based(failover::master, failover::replica)
                        .set(op::on_fallback=std::cref(callback));

    std::vector<int> res;
    ozo::request[roles](conn_info[io], "BAD QUERY"_SQL, ozo::into(res),
            std::cref(callback));
    EXPECT_CALL(callback, call(Eq(ozo::sqlstate::syntax_error_or_access_rule_violation)));
    io.run();
}

TEST(role_based, should_return_error_for_invalid_connection_info_of_all_roles) {
    using namespace ozo::literals;
    using namespace hana::literals;

    ozo::io_context io;
    auto conn_info = failover::make_role_based_connection_source(
        failover::master=ozo::connection_info("invalid connection info"),
        failover::replica=ozo::connection_info("invalid connection info")
    );
    StrictMock<callback_mock> callback;
    auto roles = failover::role_based(failover::master, failover::replica)
                        .set(op::on_fallback=std::cref(callback));

    EXPECT_CALL(callback, call(Eq(ozo::errc::connection_error))).Times(2);

    std::vector<int> res;
    ozo::request[roles](conn_info[io], "SELECT 1"_SQL + " + 1"_SQL, ozo::into(res),
            std::cref(callback));

    io.run();
}

} // namespace
