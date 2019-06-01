#include <ozo/error.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace {

TEST(connection_error, should_match_to_mapped_errors_only) {
    const auto connection_error = ozo::error_condition{ozo::errc::connection_error};
    EXPECT_EQ(connection_error, ozo::sqlstate::make_error_code(ozo::sqlstate::connection_does_not_exist));
    EXPECT_EQ(connection_error, boost::asio::error::connection_aborted);
    EXPECT_EQ(connection_error, boost::system::errc::make_error_code(boost::system::errc::io_error));
    EXPECT_EQ(connection_error, ozo::error::pq_socket_failed);
    EXPECT_NE(connection_error, ozo::error::bad_object_size);
}

TEST(database_readonly, should_match_to_mapped_errors_only) {
    const auto database_readonly = ozo::error_condition{ozo::errc::database_readonly};
    EXPECT_EQ(database_readonly, ozo::sqlstate::make_error_code(ozo::sqlstate::read_only_sql_transaction));
    EXPECT_NE(database_readonly, ozo::error::pq_socket_failed);
}

TEST(introspection_error, should_match_to_mapped_errors_only) {
    const auto introspection_error = ozo::error_condition{ozo::errc::introspection_error};
    EXPECT_EQ(introspection_error, ozo::error::bad_object_size);
    EXPECT_NE(introspection_error, ozo::error::pq_socket_failed);
}

TEST(type_mismatch, should_match_to_mapped_errors_only) {
    const auto type_mismatch = ozo::error_condition{ozo::errc::type_mismatch};
    EXPECT_EQ(type_mismatch, ozo::error::oid_type_mismatch);
    EXPECT_NE(type_mismatch, ozo::error::pq_socket_failed);
}

TEST(protocol_error, should_match_to_mapped_errors_only) {
    const auto protocol_error = ozo::error_condition{ozo::errc::protocol_error};
    EXPECT_EQ(protocol_error, ozo::error::no_sql_state_found);
    EXPECT_NE(protocol_error, ozo::error::pq_socket_failed);
}

}
