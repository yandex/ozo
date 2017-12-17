#include "apq/value.h"

#include <GUnit/GTest.h>
#include <boost/hana/map.hpp>
#include <boost/endian/conversion.hpp>

using namespace apq;
using boost::endian::native_to_big;

auto empty_map = boost::hana::make_map();

GTEST("libapq::value", "[returns type mismatch error if oid does not match the type]")
{ 
    int x;
    EXPECT_EQ(type_mismatch, convert_value(TEXTOID, nullptr, 0, empty_map, x));
}

GTEST("libapq::value", "[converts INT2OID to int16_t]")
{
    const int16_t expected = 7;
    int16_t got;

    const int16_t bytes_storage = native_to_big(expected);
    const char* bytes = reinterpret_cast<const char*>(&bytes_storage);
    const auto size = sizeof(int16_t);

    EXPECT_EQ(success, convert_value(INT2OID, bytes, size, empty_map, got));
    EXPECT_EQ(expected, got);
}

GTEST("libapq::value", "[converts INT4OID to int32_t]")
{
    const int32_t expected = 7;
    int32_t got;

    const int32_t bytes_storage = native_to_big(expected);
    const char* bytes = reinterpret_cast<const char*>(&bytes_storage);
    const auto size = sizeof(int32_t);

    EXPECT_EQ(success, convert_value(INT4OID, bytes, size, empty_map, got));
    EXPECT_EQ(expected, got);
}

GTEST("libapq::value", "[converts INT8OID to int64_t]")
{
    const int64_t expected = 7;
    int64_t got;

    const int64_t bytes_storage = native_to_big(expected);
    const char* bytes = reinterpret_cast<const char*>(&bytes_storage);
    const auto size = sizeof(int64_t);

    EXPECT_EQ(success, convert_value(INT8OID, bytes, size, empty_map, got));
    EXPECT_EQ(expected, got);
}

GTEST("libapq::value", "[converts TEXTOID to std::string]")
{
    const std::string expected = "test";
    std::string got;

    EXPECT_EQ(success, convert_value(TEXTOID, expected.data(), expected.size(), empty_map, got));
    EXPECT_EQ(expected, got);    
}
