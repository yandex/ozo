#include <apq/connection_pool.h>

#include <GUnit/GTest.h>

GTEST("libapq::connection_pool", "[constructor should not throw]") {
    EXPECT_NO_THROW(libapq::connection_pool());
}
