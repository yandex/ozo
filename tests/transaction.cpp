#include <ozo/transaction.h>

#include "connection_mock.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace {

// Test `constexpr`ness of transaction-related stuff here (so accidental breaking changes to the interface can be noticed)
constexpr auto custom_opt = ozo::make_options(ozo::transaction_options::isolation_level = ozo::isolation_level::serializable);
[[maybe_unused]] constexpr auto custom_begin = ozo::begin.with_transaction_options(custom_opt);

using namespace testing;
using namespace ozo::tests;
using namespace std::string_literals;

struct transaction : Test {
    StrictMock<connection_gmock> connection {};
    StrictMock<PGconn_mock> native_handle{};
    io_context io;
    using connection_t = decltype(make_connection(connection, io, native_handle, ozo::empty_oid_map_c));
    connection_t conn = make_connection(connection, io, native_handle, ozo::empty_oid_map_c);
    using options_t = decltype(ozo::make_options());
    options_t options = ozo::make_options();

    static_assert(
        std::is_default_constructible_v<ozo::transaction<connection_t, options_t>>,
        "transaction should be default constructible for default-constructible connection"
    );

    static_assert(
        !std::is_default_constructible_v<ozo::transaction<typename connection_t::element_type, options_t>>,
        "transaction should not be default constructible for non default-constructible connection"
    );

};

TEST_F(transaction, lowest_layer__should_return_reference_on_unwrapped_connection) {
    ozo::transaction<connection_t, options_t> t(connection_t{conn}, options);
    ASSERT_EQ(std::addressof(t.lowest_layer()), std::addressof(*conn));
    ASSERT_EQ(std::addressof(std::as_const(t).lowest_layer()), std::addressof(*conn));
}

TEST_F(transaction, should_be_in_null_state_for_default_constructible_connection_in_null_state) {
    ozo::transaction<connection_t, options_t> t;
    EXPECT_TRUE(ozo::is_null(t));
}

TEST_F(transaction, is_open__should_return_false_for_default_constructible_connection_in_null_state) {
    ozo::transaction<connection_t, options_t> t;
    EXPECT_FALSE(t.is_open());
}

TEST_F(transaction, operator_bool__should_return_false_for_default_constructible_connection_in_null_state) {
    ozo::transaction<connection_t, options_t> t;
    EXPECT_FALSE(bool(t));
}

TEST_F(transaction, should_not_be_in_null_state_for_connection_not_in_null_state) {
    ozo::transaction<connection_t, options_t> t(std::move(conn), options);
    EXPECT_FALSE(ozo::is_null(t));
}

TEST_F(transaction, native_handle__should_return_connection_native_handle) {
    ozo::transaction<connection_t, options_t> t(connection_t{conn}, options);
    EXPECT_EQ(t.native_handle(), conn->native_handle());
}

TEST_F(transaction, oid_map__should_return_reference_to_connection_oid_map) {
    ozo::transaction<connection_t, options_t> t(connection_t{conn}, options);
    EXPECT_EQ(std::addressof(t.oid_map()), std::addressof(conn->oid_map()));
}

TEST_F(transaction, get_error_context__should_return_error_context_of_connection) {
    conn->error_context_ = "the context";
    ozo::transaction<connection_t, options_t> t(std::move(conn), options);
    EXPECT_EQ(t.get_error_context(), "the context"s);
}

TEST_F(transaction, set_error_context__should_set_error_context_of_connection) {
    conn->error_context_ = "";
    ozo::transaction<connection_t, options_t> t(connection_t{conn}, options);
    t.set_error_context("the context");
    EXPECT_EQ(conn->error_context_, "the context");
}

TEST_F(transaction, get_executor__should_return_executor_of_connection) {
    ozo::transaction<connection_t, options_t> t(connection_t{conn}, options);
    EXPECT_EQ(t.get_executor(), conn->get_executor());
}

TEST_F(transaction, is_open__should_return_true_if_connection_is_open) {
    ASSERT_TRUE(conn->is_open());

    ozo::transaction<connection_t, options_t> t(std::move(conn), options);
    EXPECT_TRUE(t.is_open());
}

TEST_F(transaction, is_open__should_return_false_if_connection_is_closed) {
    conn->handle_.mock_ = nullptr;
    ASSERT_FALSE(conn->is_open());

    ozo::transaction<connection_t, options_t> t(std::move(conn), options);
    EXPECT_FALSE(t.is_open());
}

TEST_F(transaction, is_bad__should_return_result_of_underlying_connection_is_bad) {
    ozo::transaction<connection_t, options_t> t(std::move(conn), options);
    EXPECT_CALL(connection, is_bad()).WillOnce(Return(true));
    EXPECT_TRUE(t.is_bad());
    EXPECT_CALL(connection, is_bad()).WillOnce(Return(false));
    EXPECT_FALSE(t.is_bad());
}

TEST_F(transaction, operator_bool__should_return_negate_result_of_underlying_connection_is_bad) {
    ozo::transaction<connection_t, options_t> t(std::move(conn), options);
    EXPECT_CALL(connection, is_bad()).WillOnce(Return(false));
    EXPECT_TRUE(bool(t));
    EXPECT_CALL(connection, is_bad()).WillOnce(Return(true));
    EXPECT_FALSE(bool(t));
}

TEST_F(transaction, release_connection__should_return_undelying_connection) {
    ozo::transaction<connection_t, options_t> t(connection_t{conn}, options);
    EXPECT_EQ(release_connection(std::move(t)), conn);
}

TEST_F(transaction, cancel__should_call_underlying_connection_cancel) {
    ozo::transaction<connection_t, options_t> t(std::move(conn), options);
    EXPECT_CALL(connection, cancel()).WillOnce(Return());
    t.cancel();
}

}
