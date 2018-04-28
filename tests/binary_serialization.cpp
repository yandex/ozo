#include <GUnit/GTest.h>

#include <ozo/binary_serialization.h>

#include <boost/hana/members.hpp>

namespace {

struct fixed_size_struct {
    BOOST_HANA_DEFINE_STRUCT(fixed_size_struct,
        (std::int64_t, value)
    );
};

constexpr std::size_t size_of(const fixed_size_struct&) {
    return sizeof(std::int32_t) + sizeof(ozo::oid_t) + sizeof(std::int32_t) + sizeof(fixed_size_struct);
}

} // namespace

namespace ozo {

template <>
struct send_impl<fixed_size_struct> {
    template <typename M>
    static ostream& apply(ostream& out, const oid_map_t<M>& oid_map, const fixed_size_struct& in) {
        using ozo::size_of;
        send(out, oid_map, std::int32_t(1));
        send(out, oid_map, ozo::type_oid(oid_map, in.value));
        send(out, oid_map, std::int32_t(sizeof(in.value)));
        send(out, oid_map, in.value);
        return out;
    }
};

} // namespace ozo

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

    SHOULD("with fixed size struct should return bytes") {
        using namespace testing;
        fixture f;
        ozo::send(f.os, ozo::register_types<fixed_size_struct>(), fixed_size_struct {42});
        EXPECT_EQ(f.buffer, std::vector<char>({
            0, 0, 0, 1,
            0, 0, 0, 20,
            0, 0, 0, 8,
            0, 0, 0, 0,
            0, 0, 0, 42
        }));
    }

    SHOULD("with vector of fixed size struct should return bytes") {
        using namespace testing;
        fixture f;
        auto oid_map = ozo::register_types<fixed_size_struct>();
        ozo::set_type_oid<fixed_size_struct>(oid_map, 13);
        ozo::send(f.os, oid_map, std::vector<fixed_size_struct>({fixed_size_struct {42}}));
        EXPECT_EQ(f.buffer, std::vector<char>({
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
