#include <ozo/io/size_of.h>
#include <ozo/ext/std/optional.h>

#include <gtest/gtest.h>

namespace ozo::tests {

struct sized_type {};

} // namespace ozo::tests

namespace ozo {

template <>
struct size_of_impl<ozo::tests::sized_type> {
    static constexpr ozo::size_type apply(const ozo::tests::sized_type&) {
        return 42;
    }
};

} // namespace ozo

OZO_PG_DEFINE_CUSTOM_TYPE(ozo::tests::sized_type, "sized_type", dynamic_size)

namespace {

using namespace testing;

using ozo::tests::sized_type;

TEST(data_frame_size, should_add_size_of_size_type_and_size_of_data) {
    EXPECT_EQ(ozo::data_frame_size(sized_type()), sizeof(ozo::size_type) + 42);
}

TEST(data_frame_size, for_empty_optional_should_be_equal_to_size_of_size_type) {
    EXPECT_EQ(ozo::data_frame_size(OZO_STD_OPTIONAL<sized_type>()), sizeof(ozo::size_type));
}

} // namespace
