#include <ozo/connection_pool.h>

#include <GUnit/GTest.h>

GTEST("ozo::make_connection_pool") {
    SHOULD("not throw") {
        EXPECT_NO_THROW(ozo::make_connection_pool("conn info string", 1, 1));
    }
}
