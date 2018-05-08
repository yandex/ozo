#include <ozo/binary_deserialization.h>
#include "result_mock.h"

#include <GUnit/GTest.h>

BOOST_FUSION_DEFINE_STRUCT((),
    fusion_adapted_test_result,
    (std::string, text)
    (int32_t, digit)
)

namespace {

using namespace ::testing;
using ::ozo::testing::pg_result_mock;

GTEST("ozo::recv") {
    auto oid_map = ozo::empty_oid_map{};
    StrictGMock<pg_result_mock> mock{};
    ::ozo::value<pg_result_mock> value({object(mock), 0, 0});

    SHOULD("throw system_error if oid does not match the type") {
        EXPECT_INVOKE(mock, field_type, _).WillRepeatedly(Return(TEXTOID));

        int x;
        EXPECT_THROW(ozo::recv(value, oid_map, x), ozo::system_error);
    }

    SHOULD("convert BOOLOID to bool") {
        const char bytes[] = { true };

        EXPECT_INVOKE(mock, field_type, _).WillRepeatedly(Return(BOOLOID));
        EXPECT_INVOKE(mock, get_value, _, _).WillRepeatedly(Return(bytes));
        EXPECT_INVOKE(mock, get_length, _, _).WillRepeatedly(Return(sizeof(bytes)));

        bool got = false;
        ozo::recv(value, oid_map, got);
        EXPECT_TRUE(got);
    }

    SHOULD("convert FLOAT4OID to float") {
        const char bytes[] = { 0x42, 0x28, static_cast<char>(0x85), 0x1F };

        EXPECT_INVOKE(mock, field_type, _).WillRepeatedly(Return(FLOAT4OID));
        EXPECT_INVOKE(mock, get_value, _, _).WillRepeatedly(Return(bytes));
        EXPECT_INVOKE(mock, get_length, _, _).WillRepeatedly(Return(4));

        float got = 0.0;
        ozo::recv(value, oid_map, got);
        EXPECT_EQ(got, 42.13f);
    }

    SHOULD("convert INT2OID to int16_t") {
        const char bytes[] = { 0x00, 0x07 };

        EXPECT_INVOKE(mock, field_type, _).WillRepeatedly(Return(INT2OID));
        EXPECT_INVOKE(mock, get_value, _, _).WillRepeatedly(Return(bytes));
        EXPECT_INVOKE(mock, get_length, _, _).WillRepeatedly(Return(sizeof(bytes)));

        int16_t got = 0;
        ozo::recv(value, oid_map, got);
        EXPECT_EQ(7, got);
    }

    SHOULD("convert INT4OID to int32_t") {

        const char bytes[] = { 0x00, 0x00, 0x00, 0x07 };

        EXPECT_INVOKE(mock, field_type, _).WillRepeatedly(Return(INT4OID));
        EXPECT_INVOKE(mock, get_value, _, _).WillRepeatedly(Return(bytes));
        EXPECT_INVOKE(mock, get_length, _, _).WillRepeatedly(Return(sizeof(bytes)));

        int32_t got = 0;
        ozo::recv(value, oid_map, got);
        EXPECT_EQ(7, got);
    }

    SHOULD("convert INT8OID to int64_t") {

        const char bytes[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07 };

        EXPECT_INVOKE(mock, field_type, _).WillRepeatedly(Return(INT8OID));
        EXPECT_INVOKE(mock, get_value, _, _).WillRepeatedly(Return(bytes));
        EXPECT_INVOKE(mock, get_length, _, _).WillRepeatedly(Return(sizeof(bytes)));

        int64_t got = 0;
        ozo::recv(value, oid_map, got);
        EXPECT_EQ(7, got);
    }

    SHOULD("convert TEXTOID to std::string") {
        const char* bytes = "test";
        EXPECT_INVOKE(mock, field_type, _).WillRepeatedly(Return(TEXTOID));
        EXPECT_INVOKE(mock, get_value, _, _).WillRepeatedly(Return(bytes));
        EXPECT_INVOKE(mock, get_length, _, _).WillRepeatedly(Return(4));

        std::string got;
        ozo::recv(value, oid_map, got);
        EXPECT_EQ("test", got);
    }

    SHOULD("convert TEXTARRAYOID to std::vector<std::string>") {
        const char bytes[] = {
            0x00, 0x00, 0x00, 0x01, // dimention count
            0x00, 0x00, 0x00, 0x00, // data offset
            0x00, 0x00, 0x00, 0x19, // Oid
            0x00, 0x00, 0x00, 0x03, // dimention size
            0x00, 0x00, 0x00, 0x01, // dimention index
            0x00, 0x00, 0x00, 0x04, // 1st element size
            't', 'e', 's', 't',     // 1st element
            0x00, 0x00, 0x00, 0x03, // 2nd element size
            'f', 'o', 'o',          // 2ndst element
            0x00, 0x00, 0x00, 0x03, // 3rd element size
            'b', 'a', 'r',          // 3rd element
        };
        EXPECT_INVOKE(mock, field_type, _).WillRepeatedly(Return(TEXTARRAYOID));
        EXPECT_INVOKE(mock, get_value, _, _).WillRepeatedly(Return(bytes));
        EXPECT_INVOKE(mock, get_length, _, _).WillRepeatedly(Return(sizeof bytes));

        std::vector<std::string> got;
        ozo::recv(value, oid_map, got);
        EXPECT_THAT(got, ElementsAre("test", "foo", "bar"));
    }

    SHOULD("throw on multidimential arrays") {
        const char bytes[] = {
            0x00, 0x00, 0x00, 0x02, // dimention count
            0x00, 0x00, 0x00, 0x00, // data offset
            0x00, 0x00, 0x00, 0x19, // Oid
            0x00, 0x00, 0x00, 0x03, // dimention size
            0x00, 0x00, 0x00, 0x01, // dimention index
            0x00, 0x00, 0x00, 0x04, // 1st element size
            't', 'e', 's', 't',     // 1st element
            0x00, 0x00, 0x00, 0x03, // 2nd element size
            'f', 'o', 'o',          // 2ndst element
            0x00, 0x00, 0x00, 0x03, // 3rd element size
            'b', 'a', 'r',          // 3rd element
        };
        EXPECT_INVOKE(mock, field_type, _).WillRepeatedly(Return(TEXTARRAYOID));
        EXPECT_INVOKE(mock, get_value, _, _).WillRepeatedly(Return(bytes));
        EXPECT_INVOKE(mock, get_length, _, _).WillRepeatedly(Return(sizeof bytes));

        std::vector<std::string> got;
        ;
        EXPECT_THROW(ozo::recv(value, oid_map, got), std::range_error);
    }

    SHOULD("throw on inappropriate element oid") {
        const char bytes[] = {
            0x00, 0x00, 0x00, 0x01, // dimention count
            0x00, 0x00, 0x00, 0x00, // data offset
            0x00, 0x00, 0x00, 0x01, // Oid
            0x00, 0x00, 0x00, 0x03, // dimention size
            0x00, 0x00, 0x00, 0x01, // dimention index
            0x00, 0x00, 0x00, 0x04, // 1st element size
            't', 'e', 's', 't',     // 1st element
            0x00, 0x00, 0x00, 0x03, // 2nd element size
            'f', 'o', 'o',          // 2ndst element
            0x00, 0x00, 0x00, 0x03, // 3rd element size
            'b', 'a', 'r',          // 3rd element
        };
        EXPECT_INVOKE(mock, field_type, _).WillRepeatedly(Return(TEXTARRAYOID));
        EXPECT_INVOKE(mock, get_value, _, _).WillRepeatedly(Return(bytes));
        EXPECT_INVOKE(mock, get_length, _, _).WillRepeatedly(Return(sizeof bytes));

        std::vector<std::string> got;
        EXPECT_THROW(ozo::recv(value, oid_map, got), ozo::system_error);
    }

    SHOULD("throw on null element (with size -1)") {
        const char bytes[] = {
            0x00, 0x00, 0x00, 0x01, // dimention count
            0x00, 0x00, 0x00, 0x00, // data offset
            0x00, 0x00, 0x00, 0x19, // Oid
            0x00, 0x00, 0x00, 0x03, // dimention size
            0x00, 0x00, 0x00, 0x01, // dimention index
            char(0xFF), char(0xFF), char(0xFF), char(0xFF), // 1st element size
            0x00, 0x00, 0x00, 0x03, // 2nd element size
            'f', 'o', 'o',          // 2ndst element
            0x00, 0x00, 0x00, 0x03, // 3rd element size
            'b', 'a', 'r',          // 3rd element
        };
        EXPECT_INVOKE(mock, field_type, _).WillRepeatedly(Return(TEXTARRAYOID));
        EXPECT_INVOKE(mock, get_value, _, _).WillRepeatedly(Return(bytes));
        EXPECT_INVOKE(mock, get_length, _, _).WillRepeatedly(Return(sizeof bytes));

        std::vector<std::string> got;
        EXPECT_THROW(ozo::recv(value, oid_map, got), std::range_error);
    }
}


GTEST("ozo::recv_row") {
    auto oid_map = ozo::empty_oid_map{};
    StrictGMock<pg_result_mock> mock{};
    ::ozo::row<pg_result_mock> row({object(mock), 0, 0});

    SHOULD("throw range_error if size of tuple does not equal to row size") {
        std::tuple<int, std::string> out;
        EXPECT_INVOKE(mock, nfields).WillRepeatedly(Return(1));

        EXPECT_THROW(ozo::recv_row(row, oid_map, out), std::range_error);
    }

    SHOULD("convert INT4OID and TEXTOID to std::tuple<int32_t, std::string>") {

        const char int32_bytes[] = { 0x00, 0x00, 0x00, 0x07 };
        const char* string_bytes = "test";

        EXPECT_INVOKE(mock, nfields).WillRepeatedly(Return(2));

        EXPECT_INVOKE(mock, field_type, 0).WillRepeatedly(Return(INT4OID));
        EXPECT_INVOKE(mock, get_value, _, 0).WillRepeatedly(Return(int32_bytes));
        EXPECT_INVOKE(mock, get_length, _, 0).WillRepeatedly(Return(4));

        EXPECT_INVOKE(mock, field_type, 1).WillRepeatedly(Return(TEXTOID));
        EXPECT_INVOKE(mock, get_value, _, 1).WillRepeatedly(Return(string_bytes));
        EXPECT_INVOKE(mock, get_length, _, 1).WillRepeatedly(Return(4));

        std::tuple<int, std::string> got;
        ozo::recv_row(row, oid_map, got);
        EXPECT_EQ(std::make_tuple(int32_t(7), std::string("test")), got);
    }

    SHOULD("return type mismatch error if size of tuple does not equal to row size") {
        std::tuple<int, std::string> out;
        EXPECT_INVOKE(mock, nfields).WillRepeatedly(Return(1));

        EXPECT_THROW(ozo::recv_row(row, oid_map, out), std::range_error);
    }

    SHOULD("convert INT4OID and TEXTOID to fusion adapted structure") {

        const char int32_bytes[] = { 0x00, 0x00, 0x00, 0x07 };
        const char* string_bytes = "test";

        EXPECT_INVOKE(mock, nfields).WillRepeatedly(Return(2));

        EXPECT_INVOKE(mock, field_number, Eq("digit")).WillOnce(Return(0));
        EXPECT_INVOKE(mock, field_type, 0).WillRepeatedly(Return(INT4OID));
        EXPECT_INVOKE(mock, get_value, _, 0).WillRepeatedly(Return(int32_bytes));
        EXPECT_INVOKE(mock, get_length, _, 0).WillRepeatedly(Return(4));

        EXPECT_INVOKE(mock, field_number, Eq("text")).WillOnce(Return(1));
        EXPECT_INVOKE(mock, field_type, 1).WillRepeatedly(Return(TEXTOID));
        EXPECT_INVOKE(mock, get_value, _, 1).WillRepeatedly(Return(string_bytes));
        EXPECT_INVOKE(mock, get_length, _, 1).WillRepeatedly(Return(4));

        fusion_adapted_test_result got;
        ozo::recv_row(row, oid_map, got);
        EXPECT_EQ(got.digit, 7);
        EXPECT_EQ(got.text, "test");
    }

    SHOULD("throw range_error if number elements of structure does not equal to row size") {
        fusion_adapted_test_result out;
        EXPECT_INVOKE(mock, nfields).WillRepeatedly(Return(1));

        EXPECT_THROW(ozo::recv_row(row, oid_map, out), std::range_error);
    }

    SHOULD("throw range_error if column name corresponding to elements of structure does not found") {
        fusion_adapted_test_result out;
        EXPECT_INVOKE(mock, nfields).WillRepeatedly(Return(2));

        EXPECT_INVOKE(mock, field_number, _).WillRepeatedly(Return(-1));

        EXPECT_THROW(ozo::recv_row(row, oid_map, out), std::range_error);
    }

    SHOULD("throw range_error if row is unadapted and number of rows more than one") {
        std::int32_t out;
        EXPECT_INVOKE(mock, nfields).WillRepeatedly(Return(2));

        EXPECT_THROW(ozo::recv_row(row, oid_map, out), std::range_error);
    }
}

GTEST("ozo::recv_result") {
    auto oid_map = ozo::empty_oid_map{};
    StrictGMock<pg_result_mock> mock{};
    ::ozo::basic_result<pg_result_mock*> res(object(mock));

    SHOULD("convert INT4OID and TEXTOID to fusion adapted structures vector via back_inserter") {

        const char int32_bytes[] = { 0x00, 0x00, 0x00, 0x07 };
        const char* string_bytes = "test";

        EXPECT_INVOKE(mock, nfields).WillRepeatedly(Return(2));
        EXPECT_INVOKE(mock, ntuples).WillRepeatedly(Return(2));

        EXPECT_INVOKE(mock, field_number, Eq("digit")).WillRepeatedly(Return(0));
        EXPECT_INVOKE(mock, field_type, 0).WillRepeatedly(Return(INT4OID));
        EXPECT_INVOKE(mock, get_value, _, 0).WillRepeatedly(Return(int32_bytes));
        EXPECT_INVOKE(mock, get_length, _, 0).WillRepeatedly(Return(4));

        EXPECT_INVOKE(mock, field_number, Eq("text")).WillRepeatedly(Return(1));
        EXPECT_INVOKE(mock, field_type, 1).WillRepeatedly(Return(TEXTOID));
        EXPECT_INVOKE(mock, get_value, _, 1).WillRepeatedly(Return(string_bytes));
        EXPECT_INVOKE(mock, get_length, _, 1).WillRepeatedly(Return(4));

        std::vector<fusion_adapted_test_result> got;
        ozo::recv_result(res, oid_map, std::back_inserter(got));
        EXPECT_EQ(got.size(), 2);
        EXPECT_EQ(got[0].digit, 7);
        EXPECT_EQ(got[0].text, "test");
        EXPECT_EQ(got[1].digit, 7);
        EXPECT_EQ(got[1].text, "test");
    }

    SHOULD("convert INT4OID and TEXTOID to fusion adapted structures vector via iterator") {

        const char int32_bytes[] = { 0x00, 0x00, 0x00, 0x07 };
        const char* string_bytes = "test";

        EXPECT_INVOKE(mock, nfields).WillRepeatedly(Return(2));
        EXPECT_INVOKE(mock, ntuples).WillRepeatedly(Return(2));

        EXPECT_INVOKE(mock, field_number, Eq("digit")).WillRepeatedly(Return(0));
        EXPECT_INVOKE(mock, field_type, 0).WillRepeatedly(Return(INT4OID));
        EXPECT_INVOKE(mock, get_value, _, 0).WillRepeatedly(Return(int32_bytes));
        EXPECT_INVOKE(mock, get_length, _, 0).WillRepeatedly(Return(4));

        EXPECT_INVOKE(mock, field_number, Eq("text")).WillRepeatedly(Return(1));
        EXPECT_INVOKE(mock, field_type, 1).WillRepeatedly(Return(TEXTOID));
        EXPECT_INVOKE(mock, get_value, _, 1).WillRepeatedly(Return(string_bytes));
        EXPECT_INVOKE(mock, get_length, _, 1).WillRepeatedly(Return(4));

        std::vector<fusion_adapted_test_result> got;
        got.resize(2);
        ozo::recv_result(res, oid_map, got.begin());
        EXPECT_EQ(got[0].digit, 7);
        EXPECT_EQ(got[0].text, "test");
        EXPECT_EQ(got[1].digit, 7);
        EXPECT_EQ(got[1].text, "test");
    }

    SHOULD("convert INT4OID to vector via iterator") {

        const char int32_bytes[] = { 0x00, 0x00, 0x00, 0x07 };

        EXPECT_INVOKE(mock, nfields).WillRepeatedly(Return(1));
        EXPECT_INVOKE(mock, ntuples).WillRepeatedly(Return(2));

        EXPECT_INVOKE(mock, field_number, Eq("digit")).WillRepeatedly(Return(0));
        EXPECT_INVOKE(mock, field_type, 0).WillRepeatedly(Return(INT4OID));
        EXPECT_INVOKE(mock, get_value, _, 0).WillRepeatedly(Return(int32_bytes));
        EXPECT_INVOKE(mock, get_length, _, 0).WillRepeatedly(Return(4));

        std::vector<int32_t> got;
        got.resize(2);
        ozo::recv_result(res, oid_map, got.begin());
        EXPECT_THAT(got, ElementsAre(7, 7));
    }

    SHOULD("returns result result requested") {
        ::ozo::basic_result<pg_result_mock*> got;
        ozo::recv_result(res, oid_map, got);
        EXPECT_INVOKE(mock, ntuples).WillOnce(Return(2));
        EXPECT_EQ(got.size(), 2);
    }
}

} // namespace
