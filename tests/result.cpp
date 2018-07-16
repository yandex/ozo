#include "result_mock.h"

namespace {

using namespace ::testing;

using ::ozo::testing::pg_result_mock;

struct value : Test {
    StrictMock<pg_result_mock> mock{};
    ::ozo::value<pg_result_mock> v{{&mock, 1, 2}};
};

TEST_F(value, oid_should_call_field_type_with_column) {
    EXPECT_CALL(mock, field_type(2)).WillOnce(Return(0));
    v.oid();
}

TEST_F(value, oid_should_return_field_type_result) {
    EXPECT_CALL(mock, field_type(_)).WillOnce(Return(66));
    EXPECT_EQ(v.oid(), 66u);
}

TEST_F(value, is_text_should_call_field_format_with_column) {
    EXPECT_CALL(mock, field_format(2)).WillOnce(Return(::ozo::impl::result_format::text));
    v.is_text();
}

TEST_F(value, is_text_should_return_true_if_field_format_results_result_format_text) {
    EXPECT_CALL(mock, field_format(2)).WillOnce(Return(::ozo::impl::result_format::text));
    EXPECT_TRUE(v.is_text());
}

TEST_F(value, is_text_should_return_false_if_field_format_results_result_format_binary) {
    EXPECT_CALL(mock, field_format(2)).WillOnce(Return(::ozo::impl::result_format::binary));
    EXPECT_FALSE(v.is_text());
}

TEST_F(value, is_binary_should_call_field_format_with_column) {
    EXPECT_CALL(mock, field_format(2)).WillOnce(Return(::ozo::impl::result_format::text));
    v.is_binary();
}

TEST_F(value, is_binary_should_return_false_if_field_format_results_result_format_text) {
    EXPECT_CALL(mock, field_format(_)).WillOnce(Return(::ozo::impl::result_format::text));
    EXPECT_FALSE(v.is_binary());
}

TEST_F(value, is_binary_should_return_true_if_field_format_results_result_format_binary) {
    EXPECT_CALL(mock, field_format(_)).WillOnce(Return(::ozo::impl::result_format::binary));
    EXPECT_TRUE(v.is_binary());
}

TEST_F(value, data_should_call_get_value_with_row_and_column) {
    EXPECT_CALL(mock, get_value(1, 2)).WillOnce(Return(nullptr));
    v.data();
}

TEST_F(value, data_should_return_get_value_result) {
    const char* foo = "foo";
    EXPECT_CALL(mock, get_value(_, _)).WillOnce(Return(foo));
    EXPECT_EQ(v.data(), foo);
}

TEST_F(value, size_should_call_get_length_with_row_and_column) {
    EXPECT_CALL(mock, get_length(1, 2)).WillOnce(Return(0));
    v.size();
}

TEST_F(value, size_should_return_get_length_result) {
    EXPECT_CALL(mock, get_length(_, _)).WillOnce(Return(777));
    EXPECT_EQ(v.size(), 777u);
}

TEST_F(value, is_null_should_call_get_isnull_with_row_and_column) {
    EXPECT_CALL(mock, get_isnull(1, 2)).WillOnce(Return(false));
    v.is_null();
}

TEST_F(value, is_null_should_return_true_if_get_isnull_returns_true) {
    EXPECT_CALL(mock, get_isnull(_, _)).WillOnce(Return(true));
    EXPECT_TRUE(v.is_null());
}

TEST_F(value, is_null_should_return_false_if_get_isnull_returns_false) {
    EXPECT_CALL(mock, get_isnull(_, _)).WillOnce(Return(false));
    EXPECT_FALSE(v.is_null());
}

struct row : Test {
    StrictMock<pg_result_mock> mock{};
    ::ozo::row<pg_result_mock> r{{&mock, 0, 0}};
};

TEST_F(row, empty_should_return_true_if_nfields_returns_0) {
    EXPECT_CALL(mock, nfields()).WillOnce(Return(0));

    EXPECT_TRUE(r.empty());
}

TEST_F(row, empty_should_return_false_if_nfields_returns_not_0) {
    EXPECT_CALL(mock, nfields()).WillOnce(Return(1));

    EXPECT_FALSE(r.empty());
}

TEST_F(row, size_should_return_nfields_result) {
    EXPECT_CALL(mock, nfields()).WillOnce(Return(3));

    EXPECT_EQ(r.size(), 3u);
}

TEST_F(row, begin_should_return_end_if_nfields_returns_0) {
    EXPECT_CALL(mock, nfields()).WillOnce(Return(0));

    EXPECT_EQ(r.begin(), r.end());
}

TEST_F(row, begin_should_return_iterator_on_start_column) {
    EXPECT_CALL(mock, field_type(0)).WillOnce(Return(0));
    r.begin()->oid();
}

TEST_F(row, find_should_call_field_number_with_field_name) {
    const char* name = "foo";
    EXPECT_CALL(mock, field_number(Eq("foo"))).WillOnce(Return(0));

    r.find(name);
}

TEST_F(row, find_should_return_end_if_field_number_returns_minus_1) {
    EXPECT_CALL(mock, field_number(_)).WillOnce(Return(-1));
    EXPECT_CALL(mock, nfields()).WillRepeatedly(Return(100500));
    EXPECT_EQ(r.find("foo"), r.end());
}

TEST_F(row, find_should_return_iterator_on_found_column_if_field_number_returns_not_minus_1) {
    EXPECT_CALL(mock, field_number(_)).WillOnce(Return(555));
    EXPECT_CALL(mock, nfields()).WillRepeatedly(Return(100500));
    const auto i = r.find("foo");
    EXPECT_TRUE(i != r.end());
    EXPECT_CALL(mock, field_type(555)).WillOnce(Return(0));
    i->oid();
}

TEST_F(row, operator_sqbr_from_int_should_return_value_proxy_with_column_equal_to_argument) {
    EXPECT_CALL(mock, get_value(0, 42)).WillOnce(Return(nullptr));
    r[42].data();
}

TEST_F(row, at_from_int_should_return_value_proxy_if_column_number_valid) {
    EXPECT_CALL(mock, nfields()).WillRepeatedly(Return(100500));
    EXPECT_CALL(mock, get_value(0, 42)).WillOnce(Return(nullptr));
    r.at(42).data();
}

TEST_F(row, at_from_int_should_throw_std_out_of_range_if_column_number_less_than_0) {
    EXPECT_CALL(mock, nfields()).WillRepeatedly(Return(100500));
    EXPECT_THROW(r.at(-1).data(), std::out_of_range);
}

TEST_F(row, at_from_int_should_throw_std_out_of_range_if_column_number_equals_to_nfields) {
    EXPECT_CALL(mock, nfields()).WillRepeatedly(Return(10));
    EXPECT_THROW(r.at(10).data(), std::out_of_range);
}

TEST_F(row, at_from_int_should_throw_std_out_of_range_if_column_number_greater_than_nfields) {
    EXPECT_CALL(mock, nfields()).WillRepeatedly(Return(10));
    EXPECT_THROW(r.at(42).data(), std::out_of_range);
}

TEST_F(row, at_from_name_should_return_value_proxy_if_column_name_found) {
    EXPECT_CALL(mock, field_number(_)).WillOnce(Return(42));
    EXPECT_CALL(mock, nfields()).WillRepeatedly(Return(100500));
    EXPECT_CALL(mock, get_value(0, 42)).WillOnce(Return(nullptr));
    r.at("FOO").data();
}

TEST_F(row, at_from_name_should_throw_std_out_of_range_if_column_name_not_found) {
    EXPECT_CALL(mock, field_number(_)).WillOnce(Return(-1));
    EXPECT_CALL(mock, nfields()).WillRepeatedly(Return(100500));
    EXPECT_THROW(r.at("FOO").data(), std::out_of_range);
}

struct basic_result : Test {
    StrictMock<pg_result_mock> mock{};
    ::ozo::basic_result<pg_result_mock*> result{&mock};
};

TEST_F(basic_result, empty_should_return_true_if_pg_ntuples_returns_0) {
    EXPECT_CALL(mock, ntuples()).WillOnce(Return(0));

    EXPECT_TRUE(result.empty());
}

TEST_F(basic_result, empty_should_return_false_if_pg_ntuples_returns_not_0) {
    EXPECT_CALL(mock, ntuples()).WillOnce(Return(1));

    EXPECT_FALSE(result.empty());
}

TEST_F(basic_result, size_should_return_pg_ntuples_result) {
    EXPECT_CALL(mock, ntuples()).WillOnce(Return(43));

    EXPECT_EQ(result.size(), 43u);
}

TEST_F(basic_result, begin_should_return_end_if_ntuples_returns_0) {
    EXPECT_CALL(mock, ntuples()).WillOnce(Return(0));

    EXPECT_EQ(result.begin(), result.end());
}

TEST_F(basic_result, begin_should_return_iterator_on_start_column) {
    EXPECT_CALL(mock, get_value(0, _)).WillOnce(Return(nullptr));
    result.begin()->begin()->data();
}

TEST_F(basic_result, operator_sqbr_should_return_value_proxy_with_row_equal_to_argument) {
    EXPECT_CALL(mock, get_value(42, _)).WillOnce(Return(nullptr));
    result[42][0].data();
}

TEST_F(basic_result, at_should_return_row_if_row_number_valid) {
    EXPECT_CALL(mock, ntuples()).WillRepeatedly(Return(100500));
    result.at(42);
}

TEST_F(basic_result, at_should_throw_std_out_of_range_if_row_number_less_than_0) {
    EXPECT_CALL(mock, ntuples()).WillRepeatedly(Return(100500));
    EXPECT_THROW(result.at(-1), std::out_of_range);
}

TEST_F(basic_result, at_should_throw_std_out_of_range_if_row_number_equals_to_ntuples) {
    EXPECT_CALL(mock, ntuples()).WillRepeatedly(Return(10));
    EXPECT_THROW(result.at(10), std::out_of_range);
}

TEST_F(basic_result, at_should_throw_std_out_of_range_if_row_number_greater_than_ntuples) {
    EXPECT_CALL(mock, ntuples()).WillRepeatedly(Return(10));
    EXPECT_THROW(result.at(42), std::out_of_range);
}

} // namespace
