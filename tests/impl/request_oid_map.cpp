#include "test_asio.h"

#include <ozo/connection_info.h>
#include <ozo/impl/request_oid_map.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace ozo::tests {

struct custom_type1 {};
struct custom_type2 {};

} // namespace ozo::tests

OZO_PG_DEFINE_CUSTOM_TYPE(ozo::tests::custom_type1, "custom_type1")
OZO_PG_DEFINE_CUSTOM_TYPE(ozo::tests::custom_type2, "custom_type2")

namespace {

using namespace testing;
using namespace ozo::tests;

TEST(get_types_names, should_return_empty_container_for_empty_oid_map) {
    auto type_names = ozo::impl::get_types_names(ozo::empty_oid_map{});
    EXPECT_TRUE(type_names.empty());
}

TEST(get_types_names, should_return_type_names_from_oid_map) {
    auto type_names = ozo::impl::get_types_names(
        ozo::register_types<custom_type1, custom_type2>());
    EXPECT_THAT(type_names, ElementsAre("custom_type1", "custom_type2"));
}

TEST(set_oid_map, should_set_oids_for_oid_map_from_oids_result_argument) {
    auto oid_map = ozo::register_types<custom_type1, custom_type2>();
    const ozo::impl::oids_result res = {11, 22};
    ozo::impl::set_oid_map(oid_map, res);
    ozo::impl::get_types_names(ozo::empty_oid_map{});
    EXPECT_EQ(ozo::type_oid<custom_type1>(oid_map), 11u);
    EXPECT_EQ(ozo::type_oid<custom_type2>(oid_map), 22u);
}

TEST(set_oid_map, should_throw_on_oid_map_size_is_not_equal_to_oids_result_size) {
    auto oid_map = ozo::register_types<custom_type1, custom_type2>();
    const ozo::impl::oids_result res = {11};
    EXPECT_THROW(ozo::impl::set_oid_map(oid_map, res), std::length_error);
}

TEST(set_oid_map, should_throw_on_null_oid_in_oids_result) {
    auto oid_map = ozo::register_types<custom_type1, custom_type2>();
    const ozo::impl::oids_result res = {11, ozo::null_oid};
    EXPECT_THROW(ozo::impl::set_oid_map(oid_map, res), std::invalid_argument);
}

template <class OidMap = ozo::empty_oid_map>
struct connection {
    OidMap oid_map;
    std::string error_context;

    friend OidMap& get_connection_oid_map(connection& self) {
        return self.oid_map;
    }
    friend const OidMap& get_connection_oid_map(const connection& self) {
        return self.oid_map;
    }
    friend void get_connection_socket(connection&) {}
    friend void get_connection_socket(const connection&) {}
    friend void get_connection_handle(connection&) {}
    friend void get_connection_handle(const connection&) {}
    friend std::string& get_connection_error_context(connection& self) {
        return self.error_context;
    }
    friend const std::string& get_connection_error_context(const connection& self) {
        return self.error_context;
    }
    friend void get_connection_timer(connection&) {}
    friend void get_connection_timer(const connection&) {}
};

TEST(request_oid_map_op, should_call_handler_with_oid_request_failed_error_when_oid_map_length_differs_from_result_length) {
    StrictMock<callback_gmock<connection<>>> cb_mock {};
    auto operation = ozo::impl::request_oid_map_op{wrap(cb_mock)};
    operation.res_ = std::make_shared<ozo::impl::oids_result>(1);

    EXPECT_CALL(cb_mock, call(ozo::error_code(ozo::error::oid_request_failed), _)).WillOnce(Return());
    operation(ozo::error_code {}, connection {});
}

} // namespace
