#include <ozo/core/concept.h>

#include <list>
#include <vector>
#include <string>
#include <string_view>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace {

TEST(ForwardIterator, should_return_true_for_iterator_type) {
    EXPECT_TRUE(ozo::ForwardIterator<std::list<int>::iterator>);
}

TEST(ForwardIterator, should_return_false_for_not_iterator_type) {
    EXPECT_FALSE(ozo::ForwardIterator<int>);
}

TEST(Iterable, should_return_true_for_iterable_type) {
    EXPECT_TRUE(ozo::Iterable<std::vector<int>>);
}

TEST(Iterable, should_return_false_for_not_iterable_type) {
    EXPECT_FALSE(ozo::Iterable<int>);
}

TEST(RawDataWritable, should_return_true_for_type_with_mutable_data_method_and_non_const_result) {
    EXPECT_TRUE(ozo::RawDataWritable<std::string>);
}

TEST(RawDataWritable, should_return_false_for_type_without_mutable_data_method_or_non_const_result) {
    EXPECT_FALSE(ozo::RawDataWritable<std::string_view>);
}

TEST(RawDataWritable, should_return_false_for_type_with_data_point_to_more_than_a_single_byte_value) {
    EXPECT_FALSE(ozo::RawDataWritable<std::wstring>);
}

TEST(RawDataWritable, should_return_true_for_type_lvalue_reference) {
    EXPECT_TRUE(ozo::RawDataWritable<std::string&>);
}

TEST(RawDataWritable, should_return_true_for_type_rvalue_reference) {
    EXPECT_TRUE(ozo::RawDataWritable<std::string&&>);
}

TEST(RawDataWritable, should_return_false_for_type_const_reference) {
    EXPECT_FALSE(ozo::RawDataWritable<const std::string&>);
}

TEST(RawDataReadable, should_return_true_for_type_with_mutable_data_method_and_non_const_result) {
    EXPECT_TRUE(ozo::RawDataReadable<std::string>);
}

TEST(RawDataReadable, should_return_true_for_type_with_const_data_method_and_const_result) {
    EXPECT_TRUE(ozo::RawDataReadable<std::string_view>);
}

TEST(RawDataReadable, should_return_false_for_type_with_data_point_to_more_than_a_single_byte_value) {
    EXPECT_FALSE(ozo::RawDataReadable<std::wstring>);
}

TEST(RawDataReadable, should_return_true_for_type_lvalue_reference) {
    EXPECT_TRUE(ozo::RawDataReadable<std::string&>);
}

TEST(RawDataReadable, should_return_true_for_type_rvalue_reference) {
    EXPECT_TRUE(ozo::RawDataReadable<std::string&&>);
}

TEST(RawDataReadable, should_return_true_for_type_const_reference) {
    EXPECT_TRUE(ozo::RawDataReadable<const std::string&>);
}

} // namespace
