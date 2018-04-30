#include <ozo/connection_info.h>
#include <ozo/connection_pool.h>

#include <GUnit/GTest.h>

GTEST("ozo::make_connection_pool") {
    SHOULD("not throw") {
        boost::asio::io_context io;
        const ozo::connection_info<> conn_info(io, "conn info string");
        EXPECT_NO_THROW(ozo::make_connection_pool(conn_info, 1, 1));
    }
}
