#include <apq/connection_info.h>
#include <GUnit/GTest.h>


GTEST("libapq::connection_info") {
    SHOULD("return error and bad connect for invalid connection info") {
        libapq::io_context io;
        libapq::connection_info<> conn_info(io, "invalid connection info");

        libapq::get_connection(conn_info, [](libapq::error_code ec, auto conn){
            EXPECT_TRUE(ec);
            EXPECT_TRUE(libapq::connection_bad(conn));
        });

        io.run();
    }
}
