#include "result_mock.h"

#include <ozo/io/array.h>
#include <ozo/io/recv.h>
#include <ozo/ext/std.h>
#include <ozo/pg/types.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

BOOST_FUSION_DEFINE_STRUCT((),
    fusion_adapted_test_result,
    (std::string, text)
    (int32_t, digit)
)

struct hana_adapted_test_result {
    BOOST_HANA_DEFINE_STRUCT(hana_adapted_test_result,
        (std::string, text),
        (int32_t, digit)
    );
};

namespace {

using namespace testing;
using namespace ozo::tests;
using namespace std::literals;

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
    ozo::value<pg_result_mock> value{{&mock, 0, 0}};
};

TEST_F(recv, should_throw_system_error_if_oid_does_not_match_the_type) {
    const char bytes[] = "text";
    EXPECT_CALL(mock, get_isnull(_, _)).WillRepeatedly(Return(false));
    EXPECT_CALL(mock, field_type(_)).WillRepeatedly(Return(25));
    EXPECT_CALL(mock, get_value(_, _)).WillRepeatedly(Return(bytes));
    EXPECT_CALL(mock, get_length(_, _)).WillRepeatedly(Return(sizeof(bytes)));

    int x;
    EXPECT_THROW(ozo::recv(value, oid_map, x), ozo::system_error);
}

TEST_F(recv, should_convert_BOOLOID_to_bool) {
    const char bytes[] = { true };

    EXPECT_CALL(mock, field_type(_)).WillRepeatedly(Return(16));
    EXPECT_CALL(mock, get_value(_, _)).WillRepeatedly(Return(bytes));
    EXPECT_CALL(mock, get_length(_, _)).WillRepeatedly(Return(sizeof(bytes)));
    EXPECT_CALL(mock, get_isnull(_, _)).WillRepeatedly(Return(false));

    bool got = false;
    ozo::recv(value, oid_map, got);
    EXPECT_TRUE(got);
}

TEST_F(recv, should_convert_FLOAT4OID_to_float) {
    const char bytes[] = { 0x42, 0x28, static_cast<char>(0x85), 0x1F };

    EXPECT_CALL(mock, field_type(_)).WillRepeatedly(Return(700));
    EXPECT_CALL(mock, get_value(_, _)).WillRepeatedly(Return(bytes));
    EXPECT_CALL(mock, get_length(_, _)).WillRepeatedly(Return(4));
    EXPECT_CALL(mock, get_isnull(_, _)).WillRepeatedly(Return(false));

    float got = 0.0;
    ozo::recv(value, oid_map, got);
    EXPECT_EQ(got, 42.13f);
}

TEST_F(recv, should_convert_INT2OID_to_int16_t) {
    const char bytes[] = { 0x00, 0x07 };

    EXPECT_CALL(mock, field_type(_)).WillRepeatedly(Return(21));
    EXPECT_CALL(mock, get_value(_, _)).WillRepeatedly(Return(bytes));
    EXPECT_CALL(mock, get_length(_, _)).WillRepeatedly(Return(sizeof(bytes)));
    EXPECT_CALL(mock, get_isnull(_, _)).WillRepeatedly(Return(false));

    int16_t got = 0;
    ozo::recv(value, oid_map, got);
    EXPECT_EQ(7, got);
}

TEST_F(recv, should_convert_INT4OID_to_int32_t) {
    const char bytes[] = { 0x00, 0x00, 0x00, 0x07 };

    EXPECT_CALL(mock, field_type(_)).WillRepeatedly(Return(23));
    EXPECT_CALL(mock, get_value(_, _)).WillRepeatedly(Return(bytes));
    EXPECT_CALL(mock, get_length(_, _)).WillRepeatedly(Return(sizeof(bytes)));
    EXPECT_CALL(mock, get_isnull(_, _)).WillRepeatedly(Return(false));

    int32_t got = 0;
    ozo::recv(value, oid_map, got);
    EXPECT_EQ(7, got);
}

TEST_F(recv, should_convert_INT8OID_to_int64_t) {
    const char bytes[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07 };

    EXPECT_CALL(mock, field_type(_)).WillRepeatedly(Return(20));
    EXPECT_CALL(mock, get_value(_, _)).WillRepeatedly(Return(bytes));
    EXPECT_CALL(mock, get_length(_, _)).WillRepeatedly(Return(sizeof(bytes)));
    EXPECT_CALL(mock, get_isnull(_, _)).WillRepeatedly(Return(false));

    int64_t got = 0;
    ozo::recv(value, oid_map, got);
    EXPECT_EQ(7, got);
}

TEST_F(recv, should_convert_BYTEAOID_to_pg_bytea) {
    const char* bytes = "test";
    EXPECT_CALL(mock, field_type(_)).WillRepeatedly(Return(17));
    EXPECT_CALL(mock, get_value(_, _)).WillRepeatedly(Return(bytes));
    EXPECT_CALL(mock, get_length(_, _)).WillRepeatedly(Return(4));
    EXPECT_CALL(mock, get_isnull(_, _)).WillRepeatedly(Return(false));

    ozo::pg::bytea got;
    ozo::recv(value, oid_map, got);
    EXPECT_EQ("test", std::string_view(std::data(got.get()), std::size(got.get())));
}

TEST_F(recv, should_convert_TEXTOID_to_std_string) {
    const char* bytes = "test";
    EXPECT_CALL(mock, field_type(_)).WillRepeatedly(Return(25));
    EXPECT_CALL(mock, get_value(_, _)).WillRepeatedly(Return(bytes));
    EXPECT_CALL(mock, get_length(_, _)).WillRepeatedly(Return(4));
    EXPECT_CALL(mock, get_isnull(_, _)).WillRepeatedly(Return(false));

    std::string got;
    ozo::recv(value, oid_map, got);
    EXPECT_EQ("test", got);
}

TEST_F(recv, should_convert_TEXTOID_to_a_nullable_wrapped_std_string_unwrapping_that_nullable) {
    const char* bytes = "test";
    EXPECT_CALL(mock, field_type(_)).WillRepeatedly(Return(25));
    EXPECT_CALL(mock, get_value(_, _)).WillRepeatedly(Return(bytes));
    EXPECT_CALL(mock, get_length(_, _)).WillRepeatedly(Return(4));
    EXPECT_CALL(mock, get_isnull(_, _)).WillRepeatedly(Return(false));

    std::unique_ptr<std::string> got;
    ozo::recv(value, oid_map, got);
    EXPECT_TRUE(got);
    EXPECT_EQ("test", *got);
}

TEST_F(recv, should_set_nullable_to_null_for_a_null_value_of_any_type) {
    EXPECT_CALL(mock, get_length(_, _)).WillRepeatedly(Return(0));
    EXPECT_CALL(mock, field_type(_)).WillRepeatedly(Return(23));
    EXPECT_CALL(mock, get_isnull(_, _)).WillRepeatedly(Return(true));
    EXPECT_CALL(mock, get_value(_, _)).WillRepeatedly(Return(nullptr));

    auto got = std::make_unique<int>(7);
    ozo::recv(value, oid_map, got);
    EXPECT_TRUE(!got);
}

TEST_F(recv, should_throw_for_a_null_value_if_receiving_type_is_not_nullable) {
    EXPECT_CALL(mock, get_length(_, _)).WillRepeatedly(Return(0));
    EXPECT_CALL(mock, field_type(_)).WillRepeatedly(Return(25));
    EXPECT_CALL(mock, get_isnull(_, _)).WillRepeatedly(Return(true));
    EXPECT_CALL(mock, get_value(_, _)).WillRepeatedly(Return(nullptr));

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
    EXPECT_CALL(mock, field_type(_)).WillRepeatedly(Return(1009));
    EXPECT_CALL(mock, get_value(_, _)).WillRepeatedly(Return(bytes));
    EXPECT_CALL(mock, get_length(_, _)).WillRepeatedly(Return(sizeof bytes));
    EXPECT_CALL(mock, get_isnull(_, _)).WillRepeatedly(Return(false));

    std::vector<std::string> got;
    ozo::recv(value, oid_map, got);
    EXPECT_THAT(got, ElementsAre("test", "foo", "bar"));
}

TEST_F(recv, should_convert_TEXTARRAYOID_with_matched_size_to_std_array_of_std_string) {
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
    EXPECT_CALL(mock, field_type(_)).WillRepeatedly(Return(1009));
    EXPECT_CALL(mock, get_value(_, _)).WillRepeatedly(Return(bytes));
    EXPECT_CALL(mock, get_length(_, _)).WillRepeatedly(Return(sizeof bytes));
    EXPECT_CALL(mock, get_isnull(_, _)).WillRepeatedly(Return(false));

    std::array<std::string, 3> got;
    ozo::recv(value, oid_map, got);
    EXPECT_THAT(got, ElementsAre("test", "foo", "bar"));
}

TEST_F(recv, should_throw_exception_on_TEXTARRAYOID_with_greater_size_than_std_array) {
    const char bytes[] = {
        0x00, 0x00, 0x00, 0x01, // dimension count
        0x00, 0x00, 0x00, 0x00, // data offset
        0x00, 0x00, 0x00, 0x19, // Oid
        0x00, 0x00, 0x00, 0x04, // dimension size
        0x00, 0x00, 0x00, 0x01, // dimension index
        0x00, 0x00, 0x00, 0x04, // 1st element size
        't', 'e', 's', 't',     // 1st element
        0x00, 0x00, 0x00, 0x03, // 2nd element size
        'f', 'o', 'o',          // 2ndst element
        0x00, 0x00, 0x00, 0x03, // 3rd element size
        'b', 'a', 'r',          // 3rd element
    };
    EXPECT_CALL(mock, field_type(_)).WillRepeatedly(Return(1009));
    EXPECT_CALL(mock, get_value(_, _)).WillRepeatedly(Return(bytes));
    EXPECT_CALL(mock, get_length(_, _)).WillRepeatedly(Return(sizeof bytes));
    EXPECT_CALL(mock, get_isnull(_, _)).WillRepeatedly(Return(false));

    std::array<std::string, 2> got;
    EXPECT_THROW(ozo::recv(value, oid_map, got), ozo::system_error);
}

TEST_F(recv, should_throw_exception_on_TEXTARRAYOID_with_less_size_than_std_array) {
    const char bytes[] = {
        0x00, 0x00, 0x00, 0x01, // dimension count
        0x00, 0x00, 0x00, 0x00, // data offset
        0x00, 0x00, 0x00, 0x19, // Oid
        0x00, 0x00, 0x00, 0x04, // dimension size
        0x00, 0x00, 0x00, 0x01, // dimension index
        0x00, 0x00, 0x00, 0x04, // 1st element size
        't', 'e', 's', 't',     // 1st element
        0x00, 0x00, 0x00, 0x03, // 2nd element size
        'f', 'o', 'o',          // 2ndst element
        0x00, 0x00, 0x00, 0x03, // 3rd element size
        'b', 'a', 'r',          // 3rd element
    };
    EXPECT_CALL(mock, field_type(_)).WillRepeatedly(Return(1009));
    EXPECT_CALL(mock, get_value(_, _)).WillRepeatedly(Return(bytes));
    EXPECT_CALL(mock, get_length(_, _)).WillRepeatedly(Return(sizeof bytes));
    EXPECT_CALL(mock, get_isnull(_, _)).WillRepeatedly(Return(false));

    std::array<std::string, 4> got;
    EXPECT_THROW(ozo::recv(value, oid_map, got), ozo::system_error);
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
    EXPECT_CALL(mock, field_type(_)).WillRepeatedly(Return(1009));
    EXPECT_CALL(mock, get_value(_, _)).WillRepeatedly(Return(bytes));
    EXPECT_CALL(mock, get_length(_, _)).WillRepeatedly(Return(sizeof bytes));
    EXPECT_CALL(mock, get_isnull(_, _)).WillRepeatedly(Return(false));

    std::vector<std::string> got;
    EXPECT_THROW(ozo::recv(value, oid_map, got), ozo::system_error);
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
    EXPECT_CALL(mock, field_type(_)).WillRepeatedly(Return(1009));
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
    EXPECT_CALL(mock, field_type(_)).WillRepeatedly(Return(1009));
    EXPECT_CALL(mock, get_value(_, _)).WillRepeatedly(Return(bytes));
    EXPECT_CALL(mock, get_length(_, _)).WillRepeatedly(Return(sizeof bytes));
    EXPECT_CALL(mock, get_isnull(_, _)).WillRepeatedly(Return(false));

    std::vector<std::string> got;
    EXPECT_THROW(ozo::recv(value, oid_map, got), std::invalid_argument);
}

TEST_F(recv, should_throw_exception_when_size_of_integral_differs_from_given) {
    const char bytes[] = { true };

    EXPECT_CALL(mock, field_type(_)).WillRepeatedly(Return(16));
    EXPECT_CALL(mock, get_value(_, _)).WillRepeatedly(Return(bytes));
    EXPECT_CALL(mock, get_length(_, _)).WillRepeatedly(Return(sizeof(bytes) + 1));
    EXPECT_CALL(mock, get_isnull(_, _)).WillRepeatedly(Return(false));

    bool got = false;
    EXPECT_THROW(ozo::recv(value, oid_map, got), ozo::system_error);
}

TEST_F(recv, should_read_nothing_when_dimensions_count_is_zero) {
    const char bytes[] = {
        0x00, 0x00, 0x00, 0x00, // dimension count
        0x00, 0x00, 0x00, 0x00, // data offset
        0x00, 0x00, 0x00, 0x19, // Oid
    };
    EXPECT_CALL(mock, field_type(_)).WillRepeatedly(Return(1009));
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
    EXPECT_CALL(mock, field_type(_)).WillRepeatedly(Return(1009));
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
    EXPECT_CALL(mock, field_type(_)).WillRepeatedly(Return(1009));
    EXPECT_CALL(mock, get_value(_, _)).WillRepeatedly(Return(bytes));
    EXPECT_CALL(mock, get_length(_, _)).WillRepeatedly(Return(sizeof bytes));
    EXPECT_CALL(mock, get_isnull(_, _)).WillRepeatedly(Return(false));

    std::vector<std::unique_ptr<std::string>> got;
    ozo::recv(value, oid_map, got);
    EXPECT_THAT(got, ElementsAre(
        Pointee("test"s),
        Pointee("foo"s),
        Pointee("bar"s)
    ));
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
    EXPECT_CALL(mock, field_type(_)).WillRepeatedly(Return(1009));
    EXPECT_CALL(mock, get_value(_, _)).WillRepeatedly(Return(bytes));
    EXPECT_CALL(mock, get_length(_, _)).WillRepeatedly(Return(sizeof bytes));
    EXPECT_CALL(mock, get_isnull(_, _)).WillRepeatedly(Return(false));

    std::vector<std::unique_ptr<std::string>> got;
    ozo::recv(value, oid_map, got);
    EXPECT_THAT(got, ElementsAre(
        IsNull(),
        Pointee("foo"s),
        Pointee("bar"s)
    ));
}

TEST_F(recv, should_convert_NAMEOID_to_pg_name) {
    const char* bytes = "test";
    EXPECT_CALL(mock, field_type(_)).WillRepeatedly(Return(19));
    EXPECT_CALL(mock, get_value(_, _)).WillRepeatedly(Return(bytes));
    EXPECT_CALL(mock, get_length(_, _)).WillRepeatedly(Return(4));
    EXPECT_CALL(mock, get_isnull(_, _)).WillRepeatedly(Return(false));

    ozo::pg::name got;
    ozo::recv(value, oid_map, got);
    EXPECT_EQ("test", static_cast<const std::string&>(got));
}

struct recv_row : Test {
    ozo::empty_oid_map oid_map{};
    StrictMock<pg_result_mock> mock{};
    ozo::row<pg_result_mock> row{{&mock, 0, 0}};
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

    EXPECT_CALL(mock, field_type(0)).WillRepeatedly(Return(23));
    EXPECT_CALL(mock, get_value(_, 0)).WillRepeatedly(Return(int32_bytes));
    EXPECT_CALL(mock, get_length(_, 0)).WillRepeatedly(Return(4));
    EXPECT_CALL(mock, get_isnull(_, 0)).WillRepeatedly(Return(false));

    EXPECT_CALL(mock, field_type(1)).WillRepeatedly(Return(25));
    EXPECT_CALL(mock, get_value(_, 1)).WillRepeatedly(Return(string_bytes));
    EXPECT_CALL(mock, get_length(_, 1)).WillRepeatedly(Return(4));
    EXPECT_CALL(mock, get_isnull(_, 1)).WillRepeatedly(Return(false));

    std::tuple<int, std::string> got;
    ozo::recv_row(row, oid_map, got);
    EXPECT_EQ(std::make_tuple(int32_t(7), "test"s), got);
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

    EXPECT_CALL(mock, field_number(Eq("digit"s))).WillOnce(Return(0));
    EXPECT_CALL(mock, field_type(0)).WillRepeatedly(Return(23));
    EXPECT_CALL(mock, get_value(_, 0)).WillRepeatedly(Return(int32_bytes));
    EXPECT_CALL(mock, get_length(_, 0)).WillRepeatedly(Return(4));
    EXPECT_CALL(mock, get_isnull(_, 0)).WillRepeatedly(Return(false));

    EXPECT_CALL(mock, field_number(Eq("text"s))).WillOnce(Return(1));
    EXPECT_CALL(mock, field_type(1)).WillRepeatedly(Return(25));
    EXPECT_CALL(mock, get_value(_, 1)).WillRepeatedly(Return(string_bytes));
    EXPECT_CALL(mock, get_length(_, 1)).WillRepeatedly(Return(4));
    EXPECT_CALL(mock, get_isnull(_, 1)).WillRepeatedly(Return(false));

    fusion_adapted_test_result got;
    ozo::recv_row(row, oid_map, got);
    EXPECT_EQ(got.digit, 7);
    EXPECT_EQ(got.text, "test");
}

TEST_F(recv_row, should_convert_INT4OID_and_TEXTOID_to_hana_adapted_structure) {
    const char int32_bytes[] = { 0x00, 0x00, 0x00, 0x07 };
    const char* string_bytes = "test";

    EXPECT_CALL(mock, nfields()).WillRepeatedly(Return(2));

    EXPECT_CALL(mock, field_number(Eq("digit"s))).WillOnce(Return(0));
    EXPECT_CALL(mock, field_type(0)).WillRepeatedly(Return(23));
    EXPECT_CALL(mock, get_value(_, 0)).WillRepeatedly(Return(int32_bytes));
    EXPECT_CALL(mock, get_length(_, 0)).WillRepeatedly(Return(4));
    EXPECT_CALL(mock, get_isnull(_, 0)).WillRepeatedly(Return(false));

    EXPECT_CALL(mock, field_number(Eq("text"s))).WillOnce(Return(1));
    EXPECT_CALL(mock, field_type(1)).WillRepeatedly(Return(25));
    EXPECT_CALL(mock, get_value(_, 1)).WillRepeatedly(Return(string_bytes));
    EXPECT_CALL(mock, get_length(_, 1)).WillRepeatedly(Return(4));
    EXPECT_CALL(mock, get_isnull(_, 1)).WillRepeatedly(Return(false));

    hana_adapted_test_result got;
    ozo::recv_row(row, oid_map, got);
    EXPECT_EQ(got.digit, 7);
    EXPECT_EQ(got.text, "test");
}

TEST_F(recv_row, should_throw_range_error_if_number_elements_of_fusion_adapted_structure_does_not_equal_to_row_size) {
    fusion_adapted_test_result out;
    EXPECT_CALL(mock, nfields()).WillRepeatedly(Return(1));

    EXPECT_THROW(ozo::recv_row(row, oid_map, out), std::range_error);
}


TEST_F(recv_row, should_throw_range_error_if_number_elements_of_hana_adapted_structure_does_not_equal_to_row_size) {
    hana_adapted_test_result out;
    EXPECT_CALL(mock, nfields()).WillRepeatedly(Return(1));

    EXPECT_THROW(ozo::recv_row(row, oid_map, out), std::range_error);
}

TEST_F(recv_row, should_throw_range_error_if_column_name_corresponding_to_elements_of_fusion_adapted_structure_does_not_found) {
    fusion_adapted_test_result out;
    EXPECT_CALL(mock, nfields()).WillRepeatedly(Return(2));

    EXPECT_CALL(mock, field_number(_)).WillRepeatedly(Return(-1));

    EXPECT_THROW(ozo::recv_row(row, oid_map, out), std::range_error);
}

TEST_F(recv_row, should_throw_range_error_if_column_name_corresponding_to_elements_of_hana_adapted_structure_does_not_found) {
    hana_adapted_test_result out;
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
    ozo::basic_result<pg_result_mock*> res{&mock};
};

TEST_F(recv_result, send_convert_INT4OID_and_TEXTOID_to_fusion_adapted_structures_vector_via_back_inserter) {
    const char int32_bytes[] = { 0x00, 0x00, 0x00, 0x07 };
    const char* string_bytes = "test";

    EXPECT_CALL(mock, nfields()).WillRepeatedly(Return(2));
    EXPECT_CALL(mock, ntuples()).WillRepeatedly(Return(2));

    EXPECT_CALL(mock, field_number(Eq("digit"s))).WillRepeatedly(Return(0));
    EXPECT_CALL(mock, field_type(0)).WillRepeatedly(Return(23));
    EXPECT_CALL(mock, get_value(_, 0)).WillRepeatedly(Return(int32_bytes));
    EXPECT_CALL(mock, get_length(_, 0)).WillRepeatedly(Return(4));
    EXPECT_CALL(mock, get_isnull(_, 0)).WillRepeatedly(Return(false));

    EXPECT_CALL(mock, field_number(Eq("text"s))).WillRepeatedly(Return(1));
    EXPECT_CALL(mock, field_type(1)).WillRepeatedly(Return(25));
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

TEST_F(recv_result, send_convert_INT4OID_and_TEXTOID_to_hana_adapted_structures_vector_via_back_inserter) {
    const char int32_bytes[] = { 0x00, 0x00, 0x00, 0x07 };
    const char* string_bytes = "test";

    EXPECT_CALL(mock, nfields()).WillRepeatedly(Return(2));
    EXPECT_CALL(mock, ntuples()).WillRepeatedly(Return(2));

    EXPECT_CALL(mock, field_number(Eq("digit"s))).WillRepeatedly(Return(0));
    EXPECT_CALL(mock, field_type(0)).WillRepeatedly(Return(23));
    EXPECT_CALL(mock, get_value(_, 0)).WillRepeatedly(Return(int32_bytes));
    EXPECT_CALL(mock, get_length(_, 0)).WillRepeatedly(Return(4));
    EXPECT_CALL(mock, get_isnull(_, 0)).WillRepeatedly(Return(false));

    EXPECT_CALL(mock, field_number(Eq("text"s))).WillRepeatedly(Return(1));
    EXPECT_CALL(mock, field_type(1)).WillRepeatedly(Return(25));
    EXPECT_CALL(mock, get_value(_, 1)).WillRepeatedly(Return(string_bytes));
    EXPECT_CALL(mock, get_length(_, 1)).WillRepeatedly(Return(4));
    EXPECT_CALL(mock, get_isnull(_, 1)).WillRepeatedly(Return(false));

    std::vector<hana_adapted_test_result> got;
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

    EXPECT_CALL(mock, field_number(Eq("digit"s))).WillRepeatedly(Return(0));
    EXPECT_CALL(mock, field_type(0)).WillRepeatedly(Return(23));
    EXPECT_CALL(mock, get_value(_, 0)).WillRepeatedly(Return(int32_bytes));
    EXPECT_CALL(mock, get_length(_, 0)).WillRepeatedly(Return(4));
    EXPECT_CALL(mock, get_isnull(_, 0)).WillRepeatedly(Return(false));

    EXPECT_CALL(mock, field_number(Eq("text"s))).WillRepeatedly(Return(1));
    EXPECT_CALL(mock, field_type(1)).WillRepeatedly(Return(25));
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

TEST_F(recv_result, send_convert_INT4OID_and_TEXTOID_to_hana_adapted_structures_vector_via_iterator) {
    const char int32_bytes[] = { 0x00, 0x00, 0x00, 0x07 };
    const char* string_bytes = "test";

    EXPECT_CALL(mock, nfields()).WillRepeatedly(Return(2));
    EXPECT_CALL(mock, ntuples()).WillRepeatedly(Return(2));

    EXPECT_CALL(mock, field_number(Eq("digit"s))).WillRepeatedly(Return(0));
    EXPECT_CALL(mock, field_type(0)).WillRepeatedly(Return(23));
    EXPECT_CALL(mock, get_value(_, 0)).WillRepeatedly(Return(int32_bytes));
    EXPECT_CALL(mock, get_length(_, 0)).WillRepeatedly(Return(4));
    EXPECT_CALL(mock, get_isnull(_, 0)).WillRepeatedly(Return(false));

    EXPECT_CALL(mock, field_number(Eq("text"s))).WillRepeatedly(Return(1));
    EXPECT_CALL(mock, field_type(1)).WillRepeatedly(Return(25));
    EXPECT_CALL(mock, get_value(_, 1)).WillRepeatedly(Return(string_bytes));
    EXPECT_CALL(mock, get_length(_, 1)).WillRepeatedly(Return(4));
    EXPECT_CALL(mock, get_isnull(_, 1)).WillRepeatedly(Return(false));

    std::vector<hana_adapted_test_result> got;
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

    EXPECT_CALL(mock, field_number(Eq("digit"s))).WillRepeatedly(Return(0));
    EXPECT_CALL(mock, field_type(0)).WillRepeatedly(Return(23));
    EXPECT_CALL(mock, get_value(_, 0)).WillRepeatedly(Return(int32_bytes));
    EXPECT_CALL(mock, get_length(_, 0)).WillRepeatedly(Return(4));
    EXPECT_CALL(mock, get_isnull(_, 0)).WillRepeatedly(Return(false));

    std::vector<int32_t> got;
    got.resize(2);
    ozo::recv_result(res, oid_map, got.begin());
    EXPECT_THAT(got, ElementsAre(7, 7));
}

TEST_F(recv_result, send_returns_result_then_result_requested) {
    ozo::basic_result<pg_result_mock*> got;
    ozo::recv_result(res, oid_map, got);
    EXPECT_CALL(mock, ntuples()).WillOnce(Return(2));
    EXPECT_EQ(got.size(), 2u);
}

TEST_F(recv, should_convert_UUIDOID_to_uuid) {
    const char bytes[] = {
        0x12, 0x34, 0x56, 0x78,
        char(0x90), char(0xab), char(0xcd), char(0xef),
        0x12, 0x34, 0x56, 0x78,
        0x40, char(0xab), char(0xcd), char(0xef)
     };

    EXPECT_CALL(mock, field_type(_)).WillRepeatedly(Return(2950));
    EXPECT_CALL(mock, get_value(_, _)).WillRepeatedly(Return(bytes));
    EXPECT_CALL(mock, get_length(_, _)).WillRepeatedly(Return(16));
    EXPECT_CALL(mock, get_isnull(_, _)).WillRepeatedly(Return(false));

    boost::uuids::uuid result;
    ozo::recv(value, oid_map, result);
    const boost::uuids::uuid uuid = {
        0x12, 0x34, 0x56, 0x78,
        0x90, 0xab, 0xcd, 0xef,
        0x12, 0x34, 0x56, 0x78,
        0x40, 0xab, 0xcd, 0xef
    };
    EXPECT_EQ(result, uuid);
}

} // namespace
