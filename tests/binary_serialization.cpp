#include <GUnit/GTest.h>

#include <ozo/binary_serialization.h>

GTEST("ozo::send") {
    SHOULD("with std::int8_t should return as is") {
        using namespace testing;
        std::vector<char> buffer;
        ozo::send(std::int8_t(42), ozo::register_types<>(), std::back_inserter(buffer));
        EXPECT_THAT(buffer, ElementsAre(42));
    }

    SHOULD("with std::int16_t should return bytes in big-endian order") {
        using namespace testing;
        std::vector<char> buffer;
        ozo::send(std::int16_t(42), ozo::register_types<>(), std::back_inserter(buffer));
        EXPECT_THAT(buffer, ElementsAre(0, 42));
    }

    SHOULD("with std::int32_t should return bytes in big-endian order") {
        using namespace testing;
        std::vector<char> buffer;
        ozo::send(std::int32_t(42), ozo::register_types<>(), std::back_inserter(buffer));
        EXPECT_THAT(buffer, ElementsAre(0, 0, 0, 42));
    }

    SHOULD("with std::int64_t should return bytes in big-endian order") {
        using namespace testing;
        std::vector<char> buffer;
        ozo::send(std::int64_t(42), ozo::register_types<>(), std::back_inserter(buffer));
        EXPECT_THAT(buffer, ElementsAre(0, 0, 0, 0, 0, 0, 0, 42));
    }

    SHOULD("with float should return bytes in same order") {
        using namespace testing;
        std::vector<char> buffer;
        ozo::send(42.13f, ozo::register_types<>(), std::back_inserter(buffer));
        EXPECT_THAT(buffer, ElementsAre(0x1F, 0x85, 0x28, 0x42));
    }

    SHOULD("with std::string should return bytes in same order") {
        using namespace testing;
        std::vector<char> buffer;
        ozo::send(std::string("text"), ozo::register_types<>(), std::back_inserter(buffer));
        EXPECT_THAT(buffer, ElementsAre('t', 'e', 'x', 't'));
    }

    SHOULD("with std::vector<float> should return bytes with one dimension array header and values") {
        using namespace testing;
        std::vector<char> buffer;
        ozo::send(std::vector<float>({42.13f}), ozo::register_types<>(), std::back_inserter(buffer));
        EXPECT_EQ(buffer, std::vector<char>({
            0, 0, 0, 1,
            0, 0, 0, 0,
            0, 0, 2, '\xBC',
            0, 0, 0, 1,
            0, 0, 0, 0,
            0, 0, 0, 1,
            0, 0, 0, 4,
            0x1F, char(0x85), 0x28, 0x42,
        }));
    }
}
