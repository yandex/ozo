#include <apq/connection_pool.h>

#include <GUnit/GTest.h>

GTEST("apq::connection_pool", "[constructor should not throw]") {
    EXPECT_NO_THROW(apq::connection_pool());
}
