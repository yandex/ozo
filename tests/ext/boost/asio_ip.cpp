#include <ozo/io/send.h>
#include <ozo/ext/boost/asio_ip.h>
#include <ozo/pg/types.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <string_view>

namespace {

using namespace testing;

struct send_asio_ip : Test {
    std::vector<char> buffer;
    ozo::ostream os{buffer};

    ozo::empty_oid_map oid_map;
};

TEST_F(send_asio_ip, with_boost_asio_ip_network_v4_should_store_inet_binary_format) {
    const boost::asio::ip::network_v4 address {
        boost::asio::ip::make_address_v4("192.168.0.1"), 16};
    ozo::send(os, oid_map, address);
    EXPECT_EQ(buffer, std::vector<char>({
        char(0xFF), char(0xFC), char(0xA2), char(0xFE),
        char(0xC4), char(0xC8), char(0x20), char(0x00),
    }));
}

} // namespace
