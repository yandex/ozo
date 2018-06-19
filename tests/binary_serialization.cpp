#include <GUnit/GTest.h>

#include <ozo/binary_serialization.h>

GTEST("ozo::send") {

    struct fixture {
        std::vector<char> buffer;
        ozo::detail::ostreambuf obuf{buffer};
        ozo::ostream os{&obuf};
    };

    struct badbuf : std::streambuf{};

    SHOULD("with single byte type and bad ostream should throw exception") {
        using namespace testing;
        badbuf obuf;
        ozo::ostream os{&obuf};
        EXPECT_THROW(
            ozo::send(os, ozo::register_types<>(), std::int8_t(42)),
            ozo::system_error
        );
    }

    SHOULD("with multi byte type and bad ostream should throw exception") {
        using namespace testing;
        badbuf obuf;
        ozo::ostream os{&obuf};
        EXPECT_THROW(
            ozo::send(os, ozo::register_types<>(), std::int64_t(42)),
            ozo::system_error
        );
    }

    SHOULD("with std::int8_t should return as is") {
        using namespace testing;
        fixture f;
        ozo::send(f.os, ozo::register_types<>(), std::int8_t(42));
        EXPECT_THAT(f.buffer, ElementsAre(42));
    }

    SHOULD("with std::int16_t should return bytes in big-endian order") {
        using namespace testing;
        fixture f;
        ozo::send(f.os, ozo::register_types<>(), std::int16_t(42));
        EXPECT_THAT(f.buffer, ElementsAre(0, 42));
    }

    SHOULD("with std::int32_t should return bytes in big-endian order") {
        using namespace testing;
        fixture f;
        ozo::send(f.os, ozo::register_types<>(), std::int32_t(42));
        EXPECT_THAT(f.buffer, ElementsAre(0, 0, 0, 42));
    }

    SHOULD("with std::int64_t should return bytes in big-endian order") {
        using namespace testing;
        fixture f;
        ozo::send(f.os, ozo::register_types<>(), std::int64_t(42));
        EXPECT_THAT(f.buffer, ElementsAre(0, 0, 0, 0, 0, 0, 0, 42));
    }

    SHOULD("with float should return bytes in same order") {
        using namespace testing;
        fixture f;
        ozo::send(f.os, ozo::register_types<>(), 42.13f);
        EXPECT_THAT(f.buffer, ElementsAre(0x42, 0x28, 0x85, 0x1F));
    }

    SHOULD("with std::string should return bytes in same order") {
        using namespace testing;
        fixture f;
        ozo::send(f.os, ozo::register_types<>(), std::string("text"));
        EXPECT_THAT(f.buffer, ElementsAre('t', 'e', 'x', 't'));
    }

    SHOULD("with std::vector<float> should return bytes with one dimension array header and values") {
        using namespace testing;
        fixture f;
        ozo::send(f.os, ozo::register_types<>(), std::vector<float>({42.13f}));
        EXPECT_EQ(f.buffer, std::vector<char>({
            0, 0, 0, 1,
            0, 0, 0, 0,
            0, 0, 2, '\xBC',
            0, 0, 0, 1,
            0, 0, 0, 0,
            0, 0, 0, 4,
            0x42, 0x28, char(0x85), 0x1F,
        }));
    }
}
