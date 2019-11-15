#include <ozo/connection_info.h>
#include <ozo/query_builder.h>
#include <ozo/execute.h>

#include <gtest/gtest.h>

namespace {

using namespace testing;

TEST(execute, should_perform_operation_without_result) {
    using namespace ozo::literals;

    ozo::io_context io;
    ozo::connection_info conn_info(OZO_PG_TEST_CONNINFO);

    ozo::execute(conn_info[io], "BEGIN"_SQL,
        [&](ozo::error_code ec, auto conn) {
            ASSERT_FALSE(ec) << ec.message() << " | " << error_message(conn) << " | " << get_error_context(conn);
            EXPECT_FALSE(ozo::connection_bad(conn));
        });

    io.run();
}

} // namespace
