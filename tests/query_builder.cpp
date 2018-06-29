#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <ozo/query_builder.h>

#include <boost/hana/size.hpp>

#include <limits>

namespace {

namespace hana = ::boost::hana;

TEST(detail_to_string, with_0_returns_0_s) {
    using namespace hana::literals;
    EXPECT_EQ(ozo::detail::to_string(hana::size_c<0>), "0"_s);
}

TEST(detail_to_string, with_one_digit_number_returns_string_with_same_digit) {
    using namespace hana::literals;
    EXPECT_EQ(ozo::detail::to_string(hana::size_c<7>), "7"_s);
}

TEST(detail_to_string, with_two_digits_number_returns_string_with_digits_in_same_order) {
    using namespace hana::literals;
    EXPECT_EQ(ozo::detail::to_string(hana::size_c<42>), "42"_s);
}

TEST(query_builder_text, with_one_text_element_returns_input) {
    using namespace ozo::literals;
    using namespace hana::literals;
    EXPECT_EQ("SELECT 1"_SQL.text(), "SELECT 1"_s);
}

TEST(query_builder_text, with_two_text_elements_returns_concatenation) {
    using namespace ozo::literals;
    using namespace hana::literals;
    EXPECT_EQ(("SELECT 1"_SQL + " + 1"_SQL).text(), "SELECT 1 + 1"_s);
}

TEST(query_builder_text, with_text_and_int32_param_elements_returns_text_with_placeholder_for_param) {
    using namespace ozo::literals;
    using namespace hana::literals;
    EXPECT_EQ(("SELECT "_SQL + std::int32_t(42)).text(), "SELECT $1"_s);
}

TEST(query_builder_text, with_text_and_two_int32_params_elements_returns_text_with_placeholders_for_each_param) {
    using namespace ozo::literals;
    using namespace hana::literals;
    EXPECT_EQ(("SELECT "_SQL + std::int32_t(42) + " + "_SQL + std::int32_t(42)).text(), "SELECT $1 + $2"_s);
}

TEST(query_builder_text, with_std_string_text_returns_string_text) {
    EXPECT_EQ(ozo::make_query_builder(hana::make_tuple(ozo::make_query_text(std::string("SELECT 1")))).text(), "SELECT 1");
}

TEST(query_builder_text, with_std_string_text_and_params_returns_string_text_with_with_placeholders_for_each_param) {
    EXPECT_EQ((ozo::make_query_text(std::string("SELECT ")) + std::int32_t(42)
                + ozo::make_query_text(std::string(" + ")) + std::int32_t(42)).text(), "SELECT $1 + $2");
}

TEST(query_builder_params, with_one_text_element_returns_empty_tuple) {
    using namespace ozo::literals;
    EXPECT_EQ("SELECT 1"_SQL.params(), hana::tuple<>());
}

TEST(query_builder_params, with_text_and_int32_param_elements_returns_tuple_with_one_value) {
    using namespace ozo::literals;
    EXPECT_EQ(("SELECT "_SQL + std::int32_t(42)).params(), hana::make_tuple(std::int32_t(42)));
}

TEST(query_builder_params, with_text_and_not_null_pointer_param_elements_returns_tuple_with_one_value) {
    using namespace ozo::literals;
    const auto ptr = std::make_unique<std::int32_t>(42);
    const auto params = ("SELECT "_SQL + ptr.get()).params();
    EXPECT_EQ(*params[hana::size_c<0>], std::int32_t(42));
}

} // namespace

namespace ozo::testing {

struct some_type {
    std::size_t size() const {
        return 1000;
    }
};

} // namespace ozo::testing

OZO_PG_DEFINE_CUSTOM_TYPE(ozo::testing::some_type, "some_type", dynamic_size)

namespace {

TEST(query_builder_build, with_one_text_element_returns_query_with_text_equal_to_input) {
    using namespace ozo::literals;
    EXPECT_EQ(std::string_view(hana::to<const char*>("SELECT 1"_SQL.build().text)),
        "SELECT 1");
}

TEST(query_builder_build, with_one_text_element_returns_query_without_params) {
    using namespace ozo::literals;
    EXPECT_EQ("SELECT 1"_SQL.build().params, hana::tuple<>());
}

TEST(query_builder_build, with_text_and_int32_param_elements_return_query_with_1_param) {
    using namespace ozo::literals;
    EXPECT_EQ(("SELECT "_SQL + std::int32_t(42)).build().params, hana::make_tuple(42));
}

TEST(query_builder_build, with_text_and_reference_wrapper_param_element_returns_query_with_1_param) {
    using namespace ozo::literals;
    const auto value = 42.13f;
    EXPECT_EQ(("SELECT "_SQL + std::cref(value)).build().params, hana::make_tuple(std::cref(value)));
}

TEST(query_builder_build, with_text_and_ref_to_not_null_std_unique_ptr_param_element_returns_query_with_1_param) {
    using namespace ozo::literals;
    const auto ptr = std::make_unique<float>(42.13f);
    const auto params = ("SELECT "_SQL + std::cref(ptr)).build().params;
    EXPECT_EQ(decltype(hana::size(params))::value, 1);
}

TEST(query_builder_build, with_text_and_not_null_std_shared_ptr_param_element_returns_query_with_1_param) {
    using namespace ozo::literals;
    const auto ptr = std::make_shared<float>(42.13f);
    const auto params = ("SELECT "_SQL + ptr).build().params;
    EXPECT_EQ(decltype(hana::size(params))::value, 1);
}

TEST(query_builder_build, with_text_and_custom_type_param_element_returns_query_with_1_param) {
    using namespace ozo::literals;
    const auto params = ("SELECT "_SQL + ozo::testing::some_type {}).build().params;
    EXPECT_EQ(decltype(hana::size(params))::value, 1);
}

using namespace ozo::literals;
using namespace hana::literals;

constexpr auto query = "SELECT "_SQL + 42 + " + "_SQL + 13;

static_assert(query.text() == "SELECT $1 + $2"_s,
              "query_builder should generate text in compile time");

static_assert(query.params() == hana::tuple<int, int>(42, 13),
              "query_builder should generate params in compile time");

} // namespace
