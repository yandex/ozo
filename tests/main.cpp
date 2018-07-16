#include <gtest/gtest.h>
#include <gmock/gmock.h>

int main(int argc, char* argv[]) {
    testing::FLAGS_gtest_output = "xml";
    testing::FLAGS_gtest_death_test_style = "threadsafe";
    testing::InitGoogleMock(&argc, argv);

    return RUN_ALL_TESTS();
}
