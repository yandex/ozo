#include <ozo/detail/base36.h>
#include <GUnit/GTest.h>

namespace {

GTEST("ozo::detail::b36tol()") {
    SHOULD("with HV001 returns 29999809") {
        EXPECT_EQ(ozo::detail::b36tol("HV001"), 29999809);
    }

    SHOULD("with hv001 returns 29999809") {
        EXPECT_EQ(ozo::detail::b36tol("hv001"), 29999809);
    }
}

GTEST("ozo::detail::ltob36()") {
    SHOULD("with 29999809 returns HV001") {
        EXPECT_EQ("HV001", ozo::detail::ltob36(29999809));
    }
}

} // namespace
