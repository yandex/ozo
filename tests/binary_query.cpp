#include <ozo/io/binary_query.h>
#include <ozo/optional.h>

#include <iterator>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace {

namespace hana = boost::hana;

using namespace testing;

struct binary_query_params_count : Test {};

TEST_F(binary_query_params_count, without_parameters_should_be_equal_to_0) {
    const auto query = ozo::make_binary_query("", hana::make_tuple());
    EXPECT_EQ(query.params_count, 0u);
}

TEST_F(binary_query_params_count, with_more_than_0_parameters_should_be_equal_to_that_number) {
    const auto query = ozo::make_binary_query("", hana::make_tuple(true, 42, std::string("text")));
    EXPECT_EQ(query.params_count, 3u);
}

TEST_F(binary_query_params_count, from_query_concept_with_more_than_0_parameters_should_be_equal_to_that_number) {
    const auto query = ozo::make_binary_query(ozo::make_query("", true, 42, std::string("text")));
    EXPECT_EQ(query.params_count, 3u);
}

struct binary_query_text : Test {};

TEST_F(binary_query_text, should_be_equal_to_input) {
    const auto query = ozo::make_binary_query("query", hana::make_tuple());
    EXPECT_STREQ(query.text(), "query");
}

struct binary_query_types : Test {};

TEST_F(binary_query_types, for_param_should_be_equal_to_type_oid) {
    const auto query = ozo::make_binary_query("", hana::make_tuple(std::int16_t()));
    EXPECT_EQ(query.types()[0], ozo::type_traits<std::int16_t>::oid());
}

TEST_F(binary_query_types, for_nullptr_should_be_equal_to_0) {
    const auto query = ozo::make_binary_query("", hana::make_tuple(nullptr));
    EXPECT_EQ(query.types()[0], 0u);
}

TEST_F(binary_query_types, for_not_initialized_std_optional_should_be_equal_to_value_type_oid) {
    const auto query = ozo::make_binary_query("", hana::make_tuple(__OZO_STD_OPTIONAL<std::int32_t>()));
    EXPECT_EQ(query.types()[0], ozo::type_traits<std::int32_t>::oid());
}

TEST_F(binary_query_types, for_null_std_shared_ptr_should_be_equal_to_value_type_oid) {
    const auto query = ozo::make_binary_query("", hana::make_tuple(std::shared_ptr<std::int32_t>()));
    EXPECT_EQ(query.types()[0], ozo::type_traits<std::int32_t>::oid());
}

TEST_F(binary_query_types, for_null_std_unique_ptr_should_be_equal_to_value_type_oid) {
    const auto query = ozo::make_binary_query("", hana::make_tuple(std::vector<std::int32_t>()));
    EXPECT_EQ(query.types()[0], ozo::type_traits<std::vector<std::int32_t>>::oid());
}

TEST_F(binary_query_types, for_null_std_weak_ptr_should_be_equal_to_value_type_oid) {
    const auto query = ozo::make_binary_query("", hana::make_tuple(std::weak_ptr<std::int32_t>()));
    EXPECT_EQ(query.types()[0], ozo::type_traits<std::int32_t>::oid());
}

struct binary_query_formats : Test {};

TEST_F(binary_query_formats, format_of_the_param_should_be_equal_to_1) {
    const auto query = ozo::make_binary_query("", hana::make_tuple(std::int16_t()));
    EXPECT_EQ(query.formats()[0], 1);
}

struct binary_query_lengths : Test {};

TEST_F(binary_query_lengths, should_be_equal_to_parameter_binary_serialized_data_size) {
    const auto query = ozo::make_binary_query("", hana::make_tuple(std::int16_t()));
    EXPECT_EQ(query.lengths()[0], static_cast<int>(sizeof(std::int16_t)));
}

TEST_F(binary_query_lengths, for_std_string_should_be_equal_to_std_string_length) {
    const auto query = ozo::make_binary_query("", hana::make_tuple(std::string("std::string")));
    EXPECT_EQ(query.lengths()[0], 11);
}

struct binary_query_values : Test {};

TEST_F(binary_query_values, for_string_value_should_be_equal_to_input) {
    const auto query = ozo::make_binary_query("", hana::make_tuple(std::string("string")));
    EXPECT_THAT(std::vector<char>(query.values()[0], query.values()[0] + 6),
        ElementsAre('s', 't', 'r', 'i', 'n', 'g'));
}

TEST_F(binary_query_values, with_strong_typedef_wrapped_type_should_be_represented_as_underlying_type) {
    const auto query = ozo::make_binary_query("", hana::make_tuple(ozo::pg::bytea({1,2,3,4})));
    EXPECT_THAT(std::vector<char>(query.values()[0], query.values()[0] + 4),
        ElementsAre(1, 2, 3, 4));
}

TEST_F(binary_query_values, for_nullptr_should_be_equal_to_nullptr) {
    const auto query = ozo::make_binary_query("", hana::make_tuple(nullptr));
    EXPECT_EQ(query.values()[0], nullptr);
}

TEST_F(binary_query_values, for_nullopt_value_should_be_equal_to_nullptr) {
    const auto query = ozo::make_binary_query("", hana::make_tuple(__OZO_NULLOPT));
    EXPECT_EQ(query.values()[0], nullptr);
}

TEST_F(binary_query_values, for_not_initialized_std_optional_should_be_equal_to_nullptr) {
    const auto query = ozo::make_binary_query("", hana::make_tuple(__OZO_STD_OPTIONAL<std::int32_t>()));
    EXPECT_EQ(query.values()[0], nullptr);
}

TEST_F(binary_query_values, for_initialized_std_optional_value_should_be_equal_to_binary_representation) {
    const auto query = ozo::make_binary_query("", hana::make_tuple(__OZO_STD_OPTIONAL<std::string>("string")));
    EXPECT_THAT(std::vector<char>(query.values()[0], query.values()[0] + 6),
        ElementsAre('s', 't', 'r', 'i', 'n', 'g'));
}

TEST_F(binary_query_values, for_null_std_shared_ptr_value_should_be_equal_to_nullptr) {
    const auto query = ozo::make_binary_query("", hana::make_tuple(std::shared_ptr<std::int32_t>()));
    EXPECT_EQ(query.values()[0], nullptr);
}

TEST_F(binary_query_values, for_not_null_std_shared_ptr_value_should_be_equal_to_binary_representation) {
    const auto query = ozo::make_binary_query("", hana::make_tuple(std::make_shared<std::string>("string")));
    EXPECT_THAT(std::vector<char>(query.values()[0], query.values()[0] + 6),
        ElementsAre('s', 't', 'r', 'i', 'n', 'g'));
}

TEST_F(binary_query_values, for_null_std_unique_ptr_value_should_be_equal_to_nullptr) {
    const auto query = ozo::make_binary_query("", hana::make_tuple(std::unique_ptr<std::int32_t>()));
    EXPECT_EQ(query.values()[0], nullptr);
}

TEST_F(binary_query_values, for_not_null_std_unique_ptr_value_should_be_equal_to_binary_representation) {
    const auto query = ozo::make_binary_query("", hana::make_tuple(std::make_unique<std::string>("string")));
    EXPECT_THAT(std::vector<char>(query.values()[0], query.values()[0] + 6),
        ElementsAre('s', 't', 'r', 'i', 'n', 'g'));
}

TEST_F(binary_query_values, for_null_std_weak_ptr_value_should_be_equal_to_nullptr) {
    const auto query = ozo::make_binary_query("", hana::make_tuple(std::weak_ptr<std::int32_t>()));
    EXPECT_EQ(query.values()[0], nullptr);
}

TEST_F(binary_query_values, for_not_null_std_weak_ptr_value_should_be_equal_to_binary_representation) {
    const auto ptr = std::make_shared<std::string>("string");
    const auto query = ozo::make_binary_query("", hana::make_tuple(std::weak_ptr<std::string>(ptr)));
    EXPECT_THAT(std::vector<char>(query.values()[0], query.values()[0] + 6),
        ElementsAre('s', 't', 'r', 'i', 'n', 'g'));
}

TEST_F(binary_query_values, for_std_reference_wrapper_value_should_be_equal_to_binary_representation) {
    const auto value = std::string("string");
    const auto query = ozo::make_binary_query("", hana::make_tuple(std::cref(value)));
    EXPECT_THAT(std::vector<char>(query.values()[0], query.values()[0] + 6),
        ElementsAre('s', 't', 'r', 'i', 'n', 'g'));
}

} // namespace
