#include "result_mock.h"
#include <ozo/ext/std/tuple.h>
#include <ozo/ext/std/pair.h>
#include <ozo/io/composite.h>
#include <ozo/pg/types/integer.h>
#include <ozo/pg/types/text.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

struct fusion_test_struct {
    std::string string;
    std::int64_t number;
};

struct hana_test_struct {
    std::string string;
    std::int64_t number;
};

BOOST_FUSION_ADAPT_STRUCT(fusion_test_struct, string, number)
BOOST_HANA_ADAPT_STRUCT(hana_test_struct, string, number);

OZO_PG_DEFINE_CUSTOM_TYPE(fusion_test_struct, "fusion_test_struct")
OZO_PG_DEFINE_CUSTOM_TYPE(hana_test_struct, "hana_test_struct")


static bool operator == (const fusion_test_struct& rhs, const fusion_test_struct& lhs) {
    return rhs.string == lhs.string && rhs.number == lhs.number;
}

static bool operator == (const hana_test_struct& rhs, const hana_test_struct& lhs) {
    return rhs.string == lhs.string && rhs.number == lhs.number;
}

static std::ostream& operator << (std::ostream& s, const fusion_test_struct& v) {
    return s << "(\"" << v.string << "\", " << v.number << ")";
}

static std::ostream& operator << (std::ostream& s, const hana_test_struct& v) {
    return s << "(\"" << v.string << "\", " << v.number << ")";
}

namespace {

using namespace ::testing;
using namespace std::string_literals;

TEST(size_of, should_calculate_size_of_fusion_adapted_structure_with_counter_size) {
    fusion_test_struct v{"TEST", 0};
    EXPECT_EQ(
        sizeof(ozo::size_type) // Number of fields
        + sizeof(ozo::oid_t) + sizeof(ozo::size_type) // size of frame header
            + v.string.size()   // size of string data
        + sizeof(ozo::oid_t) + sizeof(ozo::size_type)  // size of frame header
            + sizeof(v.number), // size of int64_t
        ozo::size_of(v));
}

TEST(size_of, should_calculate_size_of_hana_adapted_structure_with_counter_size) {
    hana_test_struct v{"TEST", 0};
    EXPECT_EQ(
        sizeof(ozo::size_type) // Number of fields
        + sizeof(ozo::oid_t) + sizeof(ozo::size_type) // size of frame header
            + v.string.size()   // size of string data
        + sizeof(ozo::oid_t) + sizeof(ozo::size_type)  // size of frame header
            + sizeof(v.number), // size of int64_t
        ozo::size_of(v));
}

struct send_composite : Test {
    std::vector<char> buffer;
    ozo::detail::ostreambuf obuf{buffer};
    ozo::ostream os{&obuf};

    decltype(ozo::register_types<fusion_test_struct, hana_test_struct>()) oid_map;
};

TEST_F(send_composite, should_store_fusion_adapted_structure_with_number_of_fields_and_fields_frames) {
    fusion_test_struct v{"TEST", 0x0001020304050607};
    ozo::send(os, oid_map, v);
    EXPECT_EQ(buffer, std::vector<char>({
        0x00, 0x00, 0x00, 0x02, // Number of members
                                // ---- v.string frame ---
        0x00, 0x00, 0x00, 0x19, //   Oid:  TEXTOID
        0x00, 0x00, 0x00, 0x04, //   size: 4
        'T' , 'E' , 'S' , 'T' , //   data: "TEST"
                                // ---- v.number frame ---
        0x00, 0x00, 0x00, 0x14, //   Oid:  INT8OID
        0x00, 0x00, 0x00, 0x08, //   size: 8
        0x00, 0x01, 0x02, 0x03, //   data: 00 01 02 03
        0x04, 0x05, 0x06, 0x07, //         04 05 06 07
    }));
}

TEST_F(send_composite, should_store_hana_adapted_structure_with_number_of_fields_and_fields_frames) {
    hana_test_struct v{"TEST", 0x0001020304050607};
    ozo::send(os, oid_map, v);
    EXPECT_EQ(buffer, std::vector<char>({
        0x00, 0x00, 0x00, 0x02, // Number of members
                                // ---- v.string frame ---
        0x00, 0x00, 0x00, 0x19, //   Oid:  TEXTOID
        0x00, 0x00, 0x00, 0x04, //   size: 4
        'T' , 'E' , 'S' , 'T' , //   data: "TEST"
                                // ---- v.number frame ---
        0x00, 0x00, 0x00, 0x14, //   Oid:  INT8OID
        0x00, 0x00, 0x00, 0x08, //   size: 8
        0x00, 0x01, 0x02, 0x03, //   data: 00 01 02 03
        0x04, 0x05, 0x06, 0x07, //         04 05 06 07
    }));
}

TEST_F(send_composite, should_store_std_tuple_with_number_of_fields_and_fields_frames) {
    const auto v = std::make_tuple(std::string("TEST"), std::int64_t(0x0001020304050607));
    ozo::send(os, oid_map, v);
    EXPECT_EQ(buffer, std::vector<char>({
        0x00, 0x00, 0x00, 0x02, // Number of members
                                // ---- v.string frame ---
        0x00, 0x00, 0x00, 0x19, //   Oid:  TEXTOID
        0x00, 0x00, 0x00, 0x04, //   size: 4
        'T' , 'E' , 'S' , 'T' , //   data: "TEST"
                                // ---- v.number frame ---
        0x00, 0x00, 0x00, 0x14, //   Oid:  INT8OID
        0x00, 0x00, 0x00, 0x08, //   size: 8
        0x00, 0x01, 0x02, 0x03, //   data: 00 01 02 03
        0x04, 0x05, 0x06, 0x07, //         04 05 06 07
    }));
}

TEST_F(send_composite, should_store_std_pair_with_number_of_fields_and_fields_frames) {
    const auto v = std::make_pair("TEST"s, std::int64_t(0x0001020304050607));
    ozo::send(os, oid_map, v);
    EXPECT_EQ(buffer, std::vector<char>({
        0x00, 0x00, 0x00, 0x02, // Number of members
                                // ---- v.string frame ---
        0x00, 0x00, 0x00, 0x19, //   Oid:  TEXTOID
        0x00, 0x00, 0x00, 0x04, //   size: 4
        'T' , 'E' , 'S' , 'T' , //   data: "TEST"
                                // ---- v.number frame ---
        0x00, 0x00, 0x00, 0x14, //   Oid:  INT8OID
        0x00, 0x00, 0x00, 0x08, //   size: 8
        0x00, 0x01, 0x02, 0x03, //   data: 00 01 02 03
        0x04, 0x05, 0x06, 0x07, //         04 05 06 07
    }));
}

struct recv_composite : Test {
    StrictMock<ozo::tests::pg_result_mock>  mock{};
    ozo::value<ozo::tests::pg_result_mock>  value{{&mock, 0, 0}};
    decltype(ozo::register_types<fusion_test_struct, hana_test_struct>()) oid_map;
};

TEST_F(recv_composite, should_receive_fusion_adapted_structure) {
    const char bytes[] = {
        0x00, 0x00, 0x00, 0x02, // Number of members
                                // ---- v.string frame ---
        0x00, 0x00, 0x00, 0x19, //   Oid:  TEXTOID
        0x00, 0x00, 0x00, 0x04, //   size: 4
        'T' , 'E' , 'S' , 'T' , //   data: "TEST"
                                // ---- v.number frame ---
        0x00, 0x00, 0x00, 0x14, //   Oid:  INT8OID
        0x00, 0x00, 0x00, 0x08, //   size: 8
        0x00, 0x01, 0x02, 0x03, //   data: 00 01 02 03
        0x04, 0x05, 0x06, 0x07, //         04 05 06 07
    };

    EXPECT_CALL(mock, field_type(_)).WillRepeatedly(Return(0x10));
    EXPECT_CALL(mock, get_value(_, _)).WillRepeatedly(Return(bytes));
    EXPECT_CALL(mock, get_length(_, _)).WillRepeatedly(Return(sizeof(bytes)));
    EXPECT_CALL(mock, get_isnull(_, _)).WillRepeatedly(Return(false));

    fusion_test_struct got;
    ozo::set_type_oid<fusion_test_struct>(oid_map, 0x10);
    ozo::recv(value, oid_map, got);
    const fusion_test_struct expected{"TEST", 0x0001020304050607};
    EXPECT_EQ(got, expected);
}

TEST_F(recv_composite, should_receive_hana_adapted_structure) {
    const char bytes[] = {
        0x00, 0x00, 0x00, 0x02, // Number of members
                                // ---- v.string frame ---
        0x00, 0x00, 0x00, 0x19, //   Oid:  TEXTOID
        0x00, 0x00, 0x00, 0x04, //   size: 4
        'T' , 'E' , 'S' , 'T' , //   data: "TEST"
                                // ---- v.number frame ---
        0x00, 0x00, 0x00, 0x14, //   Oid:  INT8OID
        0x00, 0x00, 0x00, 0x08, //   size: 8
        0x00, 0x01, 0x02, 0x03, //   data: 00 01 02 03
        0x04, 0x05, 0x06, 0x07, //         04 05 06 07
    };

    EXPECT_CALL(mock, field_type(_)).WillRepeatedly(Return(0x10));
    EXPECT_CALL(mock, get_value(_, _)).WillRepeatedly(Return(bytes));
    EXPECT_CALL(mock, get_length(_, _)).WillRepeatedly(Return(sizeof(bytes)));
    EXPECT_CALL(mock, get_isnull(_, _)).WillRepeatedly(Return(false));

    hana_test_struct got;
    ozo::set_type_oid<hana_test_struct>(oid_map, 0x10);
    ozo::recv(value, oid_map, got);
    const hana_test_struct expected{"TEST", 0x0001020304050607};
    EXPECT_EQ(got, expected);
}

TEST_F(recv_composite, should_receive_std_tuple) {
    const char bytes[] = {
        0x00, 0x00, 0x00, 0x02, // Number of members
                                // ---- v.string frame ---
        0x00, 0x00, 0x00, 0x19, //   Oid:  TEXTOID
        0x00, 0x00, 0x00, 0x04, //   size: 4
        'T' , 'E' , 'S' , 'T' , //   data: "TEST"
                                // ---- v.number frame ---
        0x00, 0x00, 0x00, 0x14, //   Oid:  INT8OID
        0x00, 0x00, 0x00, 0x08, //   size: 8
        0x00, 0x01, 0x02, 0x03, //   data: 00 01 02 03
        0x04, 0x05, 0x06, 0x07, //         04 05 06 07
    };

    EXPECT_CALL(mock, field_type(_)).WillRepeatedly(Return(0x08C9));
    EXPECT_CALL(mock, get_value(_, _)).WillRepeatedly(Return(bytes));
    EXPECT_CALL(mock, get_length(_, _)).WillRepeatedly(Return(sizeof(bytes)));
    EXPECT_CALL(mock, get_isnull(_, _)).WillRepeatedly(Return(false));

    std::tuple<std::string, std::int64_t> got;
    ozo::set_type_oid<hana_test_struct>(oid_map, 0x10);
    ozo::recv(value, oid_map, got);
    const auto expected = std::make_tuple("TEST", 0x0001020304050607);
    EXPECT_EQ(got, expected);
}

TEST_F(recv_composite, should_receive_std_pair) {
    const char bytes[] = {
        0x00, 0x00, 0x00, 0x02, // Number of members
                                // ---- v.string frame ---
        0x00, 0x00, 0x00, 0x19, //   Oid:  TEXTOID
        0x00, 0x00, 0x00, 0x04, //   size: 4
        'T' , 'E' , 'S' , 'T' , //   data: "TEST"
                                // ---- v.number frame ---
        0x00, 0x00, 0x00, 0x14, //   Oid:  INT8OID
        0x00, 0x00, 0x00, 0x08, //   size: 8
        0x00, 0x01, 0x02, 0x03, //   data: 00 01 02 03
        0x04, 0x05, 0x06, 0x07, //         04 05 06 07
    };

    EXPECT_CALL(mock, field_type(_)).WillRepeatedly(Return(0x08C9));
    EXPECT_CALL(mock, get_value(_, _)).WillRepeatedly(Return(bytes));
    EXPECT_CALL(mock, get_length(_, _)).WillRepeatedly(Return(sizeof(bytes)));
    EXPECT_CALL(mock, get_isnull(_, _)).WillRepeatedly(Return(false));

    std::pair<std::string, std::int64_t> got;
    ozo::set_type_oid<hana_test_struct>(oid_map, 0x10);
    ozo::recv(value, oid_map, got);
    const auto expected = std::make_pair("TEST"s, 0x0001020304050607);
    EXPECT_THAT(got, expected);
}


TEST_F(recv_composite, should_throw_exception_if_wrong_number_of_fields_are_received) {
    const char bytes[] = {
        0x00, 0x00, 0x00, 0x03, // Number of members: 3 (right is 2)
                                // ---- v.string frame ---
        0x00, 0x00, 0x00, 0x19, //   Oid:  TEXTOID
        0x00, 0x00, 0x00, 0x04, //   size: 4
        'T' , 'E' , 'S' , 'T' , //   data: "TEST"
                                // ---- v.number frame ---
        0x00, 0x00, 0x00, 0x14, //   Oid:  INT8OID
        0x00, 0x00, 0x00, 0x08, //   size: 8
        0x00, 0x01, 0x02, 0x03, //   data: 00 01 02 03
        0x04, 0x05, 0x06, 0x07, //         04 05 06 07
    };

    EXPECT_CALL(mock, field_type(_)).WillRepeatedly(Return(0x08C9));
    EXPECT_CALL(mock, get_value(_, _)).WillRepeatedly(Return(bytes));
    EXPECT_CALL(mock, get_length(_, _)).WillRepeatedly(Return(sizeof(bytes)));
    EXPECT_CALL(mock, get_isnull(_, _)).WillRepeatedly(Return(false));

    std::tuple<std::string, std::int64_t> got;
    ozo::set_type_oid<hana_test_struct>(oid_map, 0x10);
    EXPECT_THROW(ozo::recv(value, oid_map, got), ozo::system_error);
}

} // namespace
