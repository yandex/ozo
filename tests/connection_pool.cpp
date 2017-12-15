#include <apq/connection_pool.h>

#include <GUnit/GTest.h>

GTEST("libapq::make_connection_pool") {
    SHOULD("not throw") {
        EXPECT_NO_THROW(libapq::make_connection_pool("conn info string", 1, 1));
    }
}
