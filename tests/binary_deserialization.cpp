#include <ozo/binary_deserialization.h>
#include "result_mock.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

BOOST_FUSION_DEFINE_STRUCT((),
    fusion_adapted_test_result,
    (std::string, text)
    (int32_t, digit)
)

namespace {

using namespace ::testing;
using ::ozo::testing::pg_result_mock;

struct read : Test {
    struct badbuf_t : std::streambuf{} badbuf;
    ozo::istream bad_istream{&badbuf};
};

TEST_F(read, with_single_byte_type_and_bad_istream_should_throw) {
    std::int8_t out;
    EXPECT_THROW(
        ozo::read(bad_istream, out),
        ozo::system_error
    );
}

TEST_F(read, with_multi_byte_type_and_bad_ostream_should_throw) {
    std::int64_t out;
    EXPECT_THROW(
        ozo::read(bad_istream, out),
        ozo::system_error
    );
}

struct recv : Test {
    ozo::empty_oid_map oid_map{};
    StrictMock<pg_result_mock> mock{};
    ::ozo::value<pg_result_mock> value{{&mock, 0, 0}};
};

TEST_F(recv, should_throw_system_error_if_oid_does_not_match_the_type) {
    EXPECT_CALL(mock, get_isnull(_, _)).WillRepeatedly(Return(false));
    EXPECT_CALL(mock, field_type(_)).WillRepeatedly(Return(TEXTOID));

    int x;
    EXPECT_THROW(ozo::recv(value, oid_map, x), ozo::system_error);
}

TEST_F(recv, should_convert_BOOLOID_to_bool) {
    const char bytes[] = { true };

    EXPECT_CALL(mock, field_type(_)).WillRepeatedly(Return(BOOLOID));
    EXPECT_CALL(mock, get_value(_, _)).WillRepeatedly(Return(bytes));
    EXPECT_CALL(mock, get_length(_, _)).WillRepeatedly(Return(sizeof(bytes)));
    EXPECT_CALL(mock, get_isnull(_, _)).WillRepeatedly(Return(false));

    bool got = false;
    ozo::recv(value, oid_map, got);
    EXPECT_TRUE(got);
}

TEST_F(recv, should_convert_FLOAT4OID_to_float) {
    const char bytes[] = { 0x42, 0x28, static_cast<char>(0x85), 0x1F };

    EXPECT_CALL(mock, field_type(_)).WillRepeatedly(Return(FLOAT4OID));
    EXPECT_CALL(mock, get_value(_, _)).WillRepeatedly(Return(bytes));
    EXPECT_CALL(mock, get_length(_, _)).WillRepeatedly(Return(4));
    EXPECT_CALL(mock, get_isnull(_, _)).WillRepeatedly(Return(false));

    float got = 0.0;
    ozo::recv(value, oid_map, got);
    EXPECT_EQ(got, 42.13f);
}

TEST_F(recv, should_convert_INT2OID_to_int16_t) {
    const char bytes[] = { 0x00, 0x07 };

    EXPECT_CALL(mock, field_type(_)).WillRepeatedly(Return(INT2OID));
    EXPECT_CALL(mock, get_value(_, _)).WillRepeatedly(Return(bytes));
    EXPECT_CALL(mock, get_length(_, _)).WillRepeatedly(Return(sizeof(bytes)));
    EXPECT_CALL(mock, get_isnull(_, _)).WillRepeatedly(Return(false));

    int16_t got = 0;
    ozo::recv(value, oid_map, got);
    EXPECT_EQ(7, got);
}

TEST_F(recv, should_convert_INT4OID_to_int32_t) {
    const char bytes[] = { 0x00, 0x00, 0x00, 0x07 };

    EXPECT_CALL(mock, field_type(_)).WillRepeatedly(Return(INT4OID));
    EXPECT_CALL(mock, get_value(_, _)).WillRepeatedly(Return(bytes));
    EXPECT_CALL(mock, get_length(_, _)).WillRepeatedly(Return(sizeof(bytes)));
    EXPECT_CALL(mock, get_isnull(_, _)).WillRepeatedly(Return(false));

    int32_t got = 0;
    ozo::recv(value, oid_map, got);
    EXPECT_EQ(7, got);
}

TEST_F(recv, should_convert_INT8OID_to_int64_t) {
    const char bytes[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07 };

    EXPECT_CALL(mock, field_type(_)).WillRepeatedly(Return(INT8OID));
    EXPECT_CALL(mock, get_value(_, _)).WillRepeatedly(Return(bytes));
    EXPECT_CALL(mock, get_length(_, _)).WillRepeatedly(Return(sizeof(bytes)));
    EXPECT_CALL(mock, get_isnull(_, _)).WillRepeatedly(Return(false));

    int64_t got = 0;
    ozo::recv(value, oid_map, got);
    EXPECT_EQ(7, got);
}

TEST_F(recv, should_convert_BYTEAOID_to_std_vector_of_char) {
    const char* bytes = "test";
    EXPECT_CALL(mock, field_type(_)).WillRepeatedly(Return(BYTEAOID));
    EXPECT_CALL(mock, get_value(_, _)).WillRepeatedly(Return(bytes));
    EXPECT_CALL(mock, get_length(_, _)).WillRepeatedly(Return(4));
    EXPECT_CALL(mock, get_isnull(_, _)).WillRepeatedly(Return(false));

    std::vector<char> got;
    ozo::recv(value, oid_map, got);
    EXPECT_EQ("test", std::string_view(std::data(got), std::size(got)));
}

TEST_F(recv, should_convert_TEXTOID_to_std_string) {
    const char* bytes = "test";
    EXPECT_CALL(mock, field_type(_)).WillRepeatedly(Return(TEXTOID));
    EXPECT_CALL(mock, get_value(_, _)).WillRepeatedly(Return(bytes));
    EXPECT_CALL(mock, get_length(_, _)).WillRepeatedly(Return(4));
    EXPECT_CALL(mock, get_isnull(_, _)).WillRepeatedly(Return(false));

    std::string got;
    ozo::recv(value, oid_map, got);
    EXPECT_EQ("test", got);
}

TEST_F(recv, should_convert_TEXTOID_to_a_nullable_wrapped_std_string_unwrapping_that_nullable) {
    const char* bytes = "test";
    EXPECT_CALL(mock, field_type(_)).WillRepeatedly(Return(TEXTOID));
    EXPECT_CALL(mock, get_value(_, _)).WillRepeatedly(Return(bytes));
    EXPECT_CALL(mock, get_length(_, _)).WillRepeatedly(Return(4));
    EXPECT_CALL(mock, get_isnull(_, _)).WillRepeatedly(Return(false));

    std::unique_ptr<std::string> got;
    ozo::recv(value, oid_map, got);
    EXPECT_TRUE(got);
    EXPECT_EQ("test", *got);
}

TEST_F(recv, should_set_nullable_to_null_for_a_null_value_of_any_type) {
    EXPECT_CALL(mock, get_isnull(_, _)).WillRepeatedly(Return(true));

    auto got = std::make_unique<int>(7);
    ozo::recv(value, oid_map, got);
    EXPECT_TRUE(!got);
}

TEST_F(recv, should_throw_for_a_null_value_if_receiving_type_is_not_nullable) {
    EXPECT_CALL(mock, field_type(_)).WillRepeatedly(Return(TEXTOID));
    EXPECT_CALL(mock, get_isnull(_, _)).WillRepeatedly(Return(true));

    std::string got;
    EXPECT_THROW(ozo::recv(value, oid_map, got), std::invalid_argument);
}

TEST_F(recv, should_convert_TEXTARRAYOID_to_std_vector_of_std_string) {
    const char bytes[] = {
        0x00, 0x00, 0x00, 0x01, // dimension count
        0x00, 0x00, 0x00, 0x00, // data offset
        0x00, 0x00, 0x00, 0x19, // Oid
        0x00, 0x00, 0x00, 0x03, // dimension size
        0x00, 0x00, 0x00, 0x01, // dimension index
        0x00, 0x00, 0x00, 0x04, // 1st element size
        't', 'e', 's', 't',     // 1st element
        0x00, 0x00, 0x00, 0x03, // 2nd element size
        'f', 'o', 'o',          // 2ndst element
        0x00, 0x00, 0x00, 0x03, // 3rd element size
        'b', 'a', 'r',          // 3rd element
    };
    EXPECT_CALL(mock, field_type(_)).WillRepeatedly(Return(TEXTARRAYOID));
    EXPECT_CALL(mock, get_value(_, _)).WillRepeatedly(Return(bytes));
    EXPECT_CALL(mock, get_length(_, _)).WillRepeatedly(Return(sizeof bytes));
    EXPECT_CALL(mock, get_isnull(_, _)).WillRepeatedly(Return(false));

    std::vector<std::string> got;
    ozo::recv(value, oid_map, got);
    EXPECT_THAT(got, ElementsAre("test", "foo", "bar"));
}

TEST_F(recv, should_throw_on_multidimential_arrays) {
    const char bytes[] = {
        0x00, 0x00, 0x00, 0x02, // dimension count
        0x00, 0x00, 0x00, 0x00, // data offset
        0x00, 0x00, 0x00, 0x19, // Oid
        0x00, 0x00, 0x00, 0x03, // dimension size
        0x00, 0x00, 0x00, 0x01, // dimension index
        0x00, 0x00, 0x00, 0x04, // 1st element size
        't', 'e', 's', 't',     // 1st element
        0x00, 0x00, 0x00, 0x03, // 2nd element size
        'f', 'o', 'o',          // 2ndst element
        0x00, 0x00, 0x00, 0x03, // 3rd element size
        'b', 'a', 'r',          // 3rd element
    };
    EXPECT_CALL(mock, field_type(_)).WillRepeatedly(Return(TEXTARRAYOID));
    EXPECT_CALL(mock, get_value(_, _)).WillRepeatedly(Return(bytes));
    EXPECT_CALL(mock, get_length(_, _)).WillRepeatedly(Return(sizeof bytes));
    EXPECT_CALL(mock, get_isnull(_, _)).WillRepeatedly(Return(false));

    std::vector<std::string> got;
    ;
    EXPECT_THROW(ozo::recv(value, oid_map, got), std::range_error);
}

TEST_F(recv, should_throw_on_inappropriate_element_oid) {
    const char bytes[] = {
        0x00, 0x00, 0x00, 0x01, // dimension count
        0x00, 0x00, 0x00, 0x00, // data offset
        0x00, 0x00, 0x00, 0x01, // Oid
        0x00, 0x00, 0x00, 0x03, // dimension size
        0x00, 0x00, 0x00, 0x01, // dimension index
        0x00, 0x00, 0x00, 0x04, // 1st element size
        't', 'e', 's', 't',     // 1st element
        0x00, 0x00, 0x00, 0x03, // 2nd element size
        'f', 'o', 'o',          // 2ndst element
        0x00, 0x00, 0x00, 0x03, // 3rd element size
        'b', 'a', 'r',          // 3rd element
    };
    EXPECT_CALL(mock, field_type(_)).WillRepeatedly(Return(TEXTARRAYOID));
    EXPECT_CALL(mock, get_value(_, _)).WillRepeatedly(Return(bytes));
    EXPECT_CALL(mock, get_length(_, _)).WillRepeatedly(Return(sizeof bytes));
    EXPECT_CALL(mock, get_isnull(_, _)).WillRepeatedly(Return(false));

    std::vector<std::string> got;
    EXPECT_THROW(ozo::recv(value, oid_map, got), ozo::system_error);
}

TEST_F(recv, should_throw_on_null_element_for_non_nullable_out_element) {
    const char bytes[] = {
        0x00, 0x00, 0x00, 0x01, // dimension count
        0x00, 0x00, 0x00, 0x00, // data offset
        0x00, 0x00, 0x00, 0x19, // Oid
        0x00, 0x00, 0x00, 0x03, // dimension size
        0x00, 0x00, 0x00, 0x01, // dimension index
        char(0xFF), char(0xFF), char(0xFF), char(0xFF), // 1st element size
        0x00, 0x00, 0x00, 0x03, // 2nd element size
        'f', 'o', 'o',          // 2ndst element
        0x00, 0x00, 0x00, 0x03, // 3rd element size
        'b', 'a', 'r',          // 3rd element
    };
    EXPECT_CALL(mock, field_type(_)).WillRepeatedly(Return(TEXTARRAYOID));
    EXPECT_CALL(mock, get_value(_, _)).WillRepeatedly(Return(bytes));
    EXPECT_CALL(mock, get_length(_, _)).WillRepeatedly(Return(sizeof bytes));
    EXPECT_CALL(mock, get_isnull(_, _)).WillRepeatedly(Return(false));

    std::vector<std::string> got;
    EXPECT_THROW(ozo::recv(value, oid_map, got), std::invalid_argument);
}

TEST_F(recv, should_throw_exception_when_size_of_integral_differs_from_given) {
    const char bytes[] = { true };

    EXPECT_CALL(mock, field_type(_)).WillRepeatedly(Return(BOOLOID));
    EXPECT_CALL(mock, get_value(_, _)).WillRepeatedly(Return(bytes));
    EXPECT_CALL(mock, get_length(_, _)).WillRepeatedly(Return(sizeof(bytes) + 1));
    EXPECT_CALL(mock, get_isnull(_, _)).WillRepeatedly(Return(false));

    bool got = false;
    EXPECT_THROW(ozo::recv(value, oid_map, got), std::range_error);
}

TEST_F(recv, should_read_nothing_when_dimensions_count_is_zero) {
    const char bytes[] = {
        0x00, 0x00, 0x00, 0x00, // dimension count
        0x00, 0x00, 0x00, 0x00, // data offset
        0x00, 0x00, 0x00, 0x19, // Oid
    };
    EXPECT_CALL(mock, field_type(_)).WillRepeatedly(Return(TEXTARRAYOID));
    EXPECT_CALL(mock, get_value(_, _)).WillRepeatedly(Return(bytes));
    EXPECT_CALL(mock, get_length(_, _)).WillRepeatedly(Return(sizeof bytes));
    EXPECT_CALL(mock, get_isnull(_, _)).WillRepeatedly(Return(false));

    std::vector<std::string> got;
    ozo::recv(value, oid_map, got);
    EXPECT_THAT(got, ElementsAre());
}

TEST_F(recv, should_read_nothing_when_dimension_size_is_zero) {
    const char bytes[] = {
        0x00, 0x00, 0x00, 0x01, // dimension count
        0x00, 0x00, 0x00, 0x00, // data offset
        0x00, 0x00, 0x00, 0x19, // Oid
        0x00, 0x00, 0x00, 0x00, // dimension size
        0x00, 0x00, 0x00, 0x01, // dimension index
    };
    EXPECT_CALL(mock, field_type(_)).WillRepeatedly(Return(TEXTARRAYOID));
    EXPECT_CALL(mock, get_value(_, _)).WillRepeatedly(Return(bytes));
    EXPECT_CALL(mock, get_length(_, _)).WillRepeatedly(Return(sizeof bytes));
    EXPECT_CALL(mock, get_isnull(_, _)).WillRepeatedly(Return(false));

    std::vector<std::string> got;
    ozo::recv(value, oid_map, got);
    EXPECT_THAT(got, ElementsAre());
}

TEST_F(recv, should_convert_TEXTARRAYOID_to_std_vector_of_std_unique_ptr_of_std_string) {
    const char bytes[] = {
        0x00, 0x00, 0x00, 0x01, // dimension count
        0x00, 0x00, 0x00, 0x00, // data offset
        0x00, 0x00, 0x00, 0x19, // Oid
        0x00, 0x00, 0x00, 0x03, // dimension size
        0x00, 0x00, 0x00, 0x01, // dimension index
        0x00, 0x00, 0x00, 0x04, // 1st element size
        't', 'e', 's', 't',     // 1st element
        0x00, 0x00, 0x00, 0x03, // 2nd element size
        'f', 'o', 'o',          // 2ndst element
        0x00, 0x00, 0x00, 0x03, // 3rd element size
        'b', 'a', 'r',          // 3rd element
    };
    EXPECT_CALL(mock, field_type(_)).WillRepeatedly(Return(TEXTARRAYOID));
    EXPECT_CALL(mock, get_value(_, _)).WillRepeatedly(Return(bytes));
    EXPECT_CALL(mock, get_length(_, _)).WillRepeatedly(Return(sizeof bytes));
    EXPECT_CALL(mock, get_isnull(_, _)).WillRepeatedly(Return(false));

    std::vector<std::unique_ptr<std::string>> got;
    ozo::recv(value, oid_map, got);
    EXPECT_THAT(got, ElementsAre(
        Pointee(std::string("test")),
        Pointee(std::string("foo")),
        Pointee(std::string("bar"))));
}

TEST_F(recv, should_reset_nullable_on_null_element) {
    const char bytes[] = {
        0x00, 0x00, 0x00, 0x01, // dimension count
        0x00, 0x00, 0x00, 0x00, // data offset
        0x00, 0x00, 0x00, 0x19, // Oid
        0x00, 0x00, 0x00, 0x03, // dimension size
        0x00, 0x00, 0x00, 0x01, // dimension index
        char(0xFF), char(0xFF), char(0xFF), char(0xFF), // 1st element size
        0x00, 0x00, 0x00, 0x03, // 2nd element size
        'f', 'o', 'o',          // 2ndst element
        0x00, 0x00, 0x00, 0x03, // 3rd element size
        'b', 'a', 'r',          // 3rd element
    };
    EXPECT_CALL(mock, field_type(_)).WillRepeatedly(Return(TEXTARRAYOID));
    EXPECT_CALL(mock, get_value(_, _)).WillRepeatedly(Return(bytes));
    EXPECT_CALL(mock, get_length(_, _)).WillRepeatedly(Return(sizeof bytes));
    EXPECT_CALL(mock, get_isnull(_, _)).WillRepeatedly(Return(false));

    std::vector<std::unique_ptr<std::string>> got;
    ozo::recv(value, oid_map, got);
    EXPECT_THAT(got, ElementsAre(
        IsNull(),
        Pointee(std::string("foo")),
        Pointee(std::string("bar"))));
}

struct recv_row : Test {
    ozo::empty_oid_map oid_map{};
    StrictMock<pg_result_mock> mock{};
    ::ozo::row<pg_result_mock> row{{&mock, 0, 0}};
};

TEST_F(recv_row, should_throw_range_error_if_size_of_tuple_does_not_equal_to_row_size) {
    std::tuple<int, std::string> out;
    EXPECT_CALL(mock, nfields()).WillRepeatedly(Return(1));

    EXPECT_THROW(ozo::recv_row(row, oid_map, out), std::range_error);
}

TEST_F(recv_row, should_convert_INT4OID_and_TEXTOID_to_std_tuple_int32_t_std_string) {
    const char int32_bytes[] = { 0x00, 0x00, 0x00, 0x07 };
    const char* string_bytes = "test";

    EXPECT_CALL(mock, nfields()).WillRepeatedly(Return(2));

    EXPECT_CALL(mock, field_type(0)).WillRepeatedly(Return(INT4OID));
    EXPECT_CALL(mock, get_value(_, 0)).WillRepeatedly(Return(int32_bytes));
    EXPECT_CALL(mock, get_length(_, 0)).WillRepeatedly(Return(4));
    EXPECT_CALL(mock, get_isnull(_, 0)).WillRepeatedly(Return(false));

    EXPECT_CALL(mock, field_type(1)).WillRepeatedly(Return(TEXTOID));
    EXPECT_CALL(mock, get_value(_, 1)).WillRepeatedly(Return(string_bytes));
    EXPECT_CALL(mock, get_length(_, 1)).WillRepeatedly(Return(4));
    EXPECT_CALL(mock, get_isnull(_, 1)).WillRepeatedly(Return(false));

    std::tuple<int, std::string> got;
    ozo::recv_row(row, oid_map, got);
    EXPECT_EQ(std::make_tuple(int32_t(7), std::string("test")), got);
}

TEST_F(recv_row, should_return_type_mismatch_error_if_size_of_tuple_does_not_equal_to_row_size) {
    std::tuple<int, std::string> out;
    EXPECT_CALL(mock, nfields()).WillRepeatedly(Return(1));

    EXPECT_THROW(ozo::recv_row(row, oid_map, out), std::range_error);
}

TEST_F(recv_row, should_convert_INT4OID_and_TEXTOID_to_fusion_adapted_structure) {
    const char int32_bytes[] = { 0x00, 0x00, 0x00, 0x07 };
    const char* string_bytes = "test";

    EXPECT_CALL(mock, nfields()).WillRepeatedly(Return(2));

    EXPECT_CALL(mock, field_number(Eq("digit"))).WillOnce(Return(0));
    EXPECT_CALL(mock, field_type(0)).WillRepeatedly(Return(INT4OID));
    EXPECT_CALL(mock, get_value(_, 0)).WillRepeatedly(Return(int32_bytes));
    EXPECT_CALL(mock, get_length(_, 0)).WillRepeatedly(Return(4));
    EXPECT_CALL(mock, get_isnull(_, 0)).WillRepeatedly(Return(false));

    EXPECT_CALL(mock, field_number(Eq("text"))).WillOnce(Return(1));
    EXPECT_CALL(mock, field_type(1)).WillRepeatedly(Return(TEXTOID));
    EXPECT_CALL(mock, get_value(_, 1)).WillRepeatedly(Return(string_bytes));
    EXPECT_CALL(mock, get_length(_, 1)).WillRepeatedly(Return(4));
    EXPECT_CALL(mock, get_isnull(_, 1)).WillRepeatedly(Return(false));

    fusion_adapted_test_result got;
    ozo::recv_row(row, oid_map, got);
    EXPECT_EQ(got.digit, 7);
    EXPECT_EQ(got.text, "test");
}

TEST_F(recv_row, should_throw_range_error_if_number_elements_of_structure_does_not_equal_to_row_size) {
    fusion_adapted_test_result out;
    EXPECT_CALL(mock, nfields()).WillRepeatedly(Return(1));

    EXPECT_THROW(ozo::recv_row(row, oid_map, out), std::range_error);
}

TEST_F(recv_row, should_throw_range_error_if_column_name_corresponding_to_elements_of_structure_does_not_found) {
    fusion_adapted_test_result out;
    EXPECT_CALL(mock, nfields()).WillRepeatedly(Return(2));

    EXPECT_CALL(mock, field_number(_)).WillRepeatedly(Return(-1));

    EXPECT_THROW(ozo::recv_row(row, oid_map, out), std::range_error);
}

TEST_F(recv_row, should_throw_range_error_if_row_is_unadapted_and_number_of_rows_more_than_one) {
    std::int32_t out;
    EXPECT_CALL(mock, nfields()).WillRepeatedly(Return(2));

    EXPECT_THROW(ozo::recv_row(row, oid_map, out), std::range_error);
}

struct recv_result : Test {
    ozo::empty_oid_map oid_map{};
    StrictMock<pg_result_mock> mock{};
    ::ozo::basic_result<pg_result_mock*> res{&mock};
};

TEST_F(recv_result, send_convert_INT4OID_and_TEXTOID_to_fusion_adapted_structures_vector_via_back_inserter) {
    const char int32_bytes[] = { 0x00, 0x00, 0x00, 0x07 };
    const char* string_bytes = "test";

    EXPECT_CALL(mock, nfields()).WillRepeatedly(Return(2));
    EXPECT_CALL(mock, ntuples()).WillRepeatedly(Return(2));

    EXPECT_CALL(mock, field_number(Eq("digit"))).WillRepeatedly(Return(0));
    EXPECT_CALL(mock, field_type(0)).WillRepeatedly(Return(INT4OID));
    EXPECT_CALL(mock, get_value(_, 0)).WillRepeatedly(Return(int32_bytes));
    EXPECT_CALL(mock, get_length(_, 0)).WillRepeatedly(Return(4));
    EXPECT_CALL(mock, get_isnull(_, 0)).WillRepeatedly(Return(false));

    EXPECT_CALL(mock, field_number(Eq("text"))).WillRepeatedly(Return(1));
    EXPECT_CALL(mock, field_type(1)).WillRepeatedly(Return(TEXTOID));
    EXPECT_CALL(mock, get_value(_, 1)).WillRepeatedly(Return(string_bytes));
    EXPECT_CALL(mock, get_length(_, 1)).WillRepeatedly(Return(4));
    EXPECT_CALL(mock, get_isnull(_, 1)).WillRepeatedly(Return(false));

    std::vector<fusion_adapted_test_result> got;
    ozo::recv_result(res, oid_map, std::back_inserter(got));
    EXPECT_EQ(got.size(), 2u);
    EXPECT_EQ(got[0].digit, 7);
    EXPECT_EQ(got[0].text, "test");
    EXPECT_EQ(got[1].digit, 7);
    EXPECT_EQ(got[1].text, "test");
}

TEST_F(recv_result, send_convert_INT4OID_and_TEXTOID_to_fusion_adapted_structures_vector_via_iterator) {
    const char int32_bytes[] = { 0x00, 0x00, 0x00, 0x07 };
    const char* string_bytes = "test";

    EXPECT_CALL(mock, nfields()).WillRepeatedly(Return(2));
    EXPECT_CALL(mock, ntuples()).WillRepeatedly(Return(2));

    EXPECT_CALL(mock, field_number(Eq("digit"))).WillRepeatedly(Return(0));
    EXPECT_CALL(mock, field_type(0)).WillRepeatedly(Return(INT4OID));
    EXPECT_CALL(mock, get_value(_, 0)).WillRepeatedly(Return(int32_bytes));
    EXPECT_CALL(mock, get_length(_, 0)).WillRepeatedly(Return(4));
    EXPECT_CALL(mock, get_isnull(_, 0)).WillRepeatedly(Return(false));

    EXPECT_CALL(mock, field_number(Eq("text"))).WillRepeatedly(Return(1));
    EXPECT_CALL(mock, field_type(1)).WillRepeatedly(Return(TEXTOID));
    EXPECT_CALL(mock, get_value(_, 1)).WillRepeatedly(Return(string_bytes));
    EXPECT_CALL(mock, get_length(_, 1)).WillRepeatedly(Return(4));
    EXPECT_CALL(mock, get_isnull(_, 1)).WillRepeatedly(Return(false));

    std::vector<fusion_adapted_test_result> got;
    got.resize(2);
    ozo::recv_result(res, oid_map, got.begin());
    EXPECT_EQ(got[0].digit, 7);
    EXPECT_EQ(got[0].text, "test");
    EXPECT_EQ(got[1].digit, 7);
    EXPECT_EQ(got[1].text, "test");
}

TEST_F(recv_result, send_convert_INT4OID_to_vector_via_iterator) {
    const char int32_bytes[] = { 0x00, 0x00, 0x00, 0x07 };

    EXPECT_CALL(mock, nfields()).WillRepeatedly(Return(1));
    EXPECT_CALL(mock, ntuples()).WillRepeatedly(Return(2));

    EXPECT_CALL(mock, field_number(Eq("digit"))).WillRepeatedly(Return(0));
    EXPECT_CALL(mock, field_type(0)).WillRepeatedly(Return(INT4OID));
    EXPECT_CALL(mock, get_value(_, 0)).WillRepeatedly(Return(int32_bytes));
    EXPECT_CALL(mock, get_length(_, 0)).WillRepeatedly(Return(4));
    EXPECT_CALL(mock, get_isnull(_, 0)).WillRepeatedly(Return(false));

    std::vector<int32_t> got;
    got.resize(2);
    ozo::recv_result(res, oid_map, got.begin());
    EXPECT_THAT(got, ElementsAre(7, 7));
}

TEST_F(recv_result, send_returns_result_then_result_requested) {
    ::ozo::basic_result<pg_result_mock*> got;
    ozo::recv_result(res, oid_map, got);
    EXPECT_CALL(mock, ntuples()).WillOnce(Return(2));
    EXPECT_EQ(got.size(), 2u);
}

} // namespace
