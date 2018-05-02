#include <GUnit/GTest.h>

#include <ozo/binary_serialization.h>

#include <boost/hana/members.hpp>

namespace {

struct fixed_size_struct {
    std::int64_t value;
};

template <class OidMapT, class OutIteratorT>
constexpr OutIteratorT send(const fixed_size_struct& value, const OidMapT& map, OutIteratorT out) {
    using ozo::send;
    out = send(std::int32_t(1), map, out);
    out = send(ozo::type_oid(map, value.value), map, out);
    out = send(std::int32_t(sizeof(value.value)), map, out);
    out = send(value.value, map, out);
    return out;
}

constexpr std::size_t size_of(const fixed_size_struct&) {
    return sizeof(std::int32_t) + sizeof(ozo::oid_t) + sizeof(std::int32_t) + sizeof(fixed_size_struct);
}

} // namespace

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
        EXPECT_THAT(buffer, ElementsAre(0x42, 0x28, 0x85, 0x1F));
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
            0, 0, 0, 4,
            0x42, 0x28, char(0x85), 0x1F,
        }));
    }

    SHOULD("with fixed size struct should return bytes") {
        using namespace testing;
        using ozo::send;
        std::vector<char> buffer;
        const fixed_size_struct value {42};
        send(value, ozo::register_types<fixed_size_struct>(), std::back_inserter(buffer));
        EXPECT_EQ(buffer, std::vector<char>({
            0, 0, 0, 1,
            0, 0, 0, 20,
            0, 0, 0, 8,
            0, 0, 0, 0,
            0, 0, 0, 42
        }));
    }

    SHOULD("with vector of fixed size struct should return bytes") {
        using namespace testing;
        using ozo::send;
        std::vector<char> buffer;
        const std::vector<fixed_size_struct> value({fixed_size_struct {42}});
        auto oid_map = ozo::register_types<fixed_size_struct>();
        ozo::set_type_oid<fixed_size_struct>(oid_map, 13);
        send(value, oid_map, std::back_inserter(buffer));
        EXPECT_EQ(buffer, std::vector<char>({
            0, 0, 0, 1,
            0, 0, 0, 0,
            0, 0, 0, 13,
            0, 0, 0, 1,
            0, 0, 0, 0,
            0, 0, 0, 20,
            0, 0, 0, 1,
            0, 0, 0, 20,
            0, 0, 0, 8,
            0, 0, 0, 0,
            0, 0, 0, 42
        }));
    }
}
