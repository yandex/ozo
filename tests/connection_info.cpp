#include <ozo/connection_info.h>

#include <GUnit/GTest.h>

GTEST("ozo::connection_info") {
    namespace asio = boost::asio;

    SHOULD("return error and bad connect for invalid connection info") {
        ozo::io_context io;
        ozo::connection_info<> conn_info(io, "invalid connection info");

        ozo::get_connection(conn_info, [](ozo::error_code ec, auto conn){
            EXPECT_TRUE(ec);
            EXPECT_TRUE(ozo::connection_bad(conn));
        });

        io.run();
    }
}
