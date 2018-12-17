#include <connection_mock.h>
#include <test_error.h>

#include <ozo/cancel.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>


namespace {

struct io_context_mock : ozo::tests::execution_context {
    ozo::tests::steady_timer_gmock* timer_;
    io_context_mock(ozo::tests::executor_mock& executor,
        ozo::tests::strand_executor_service_mock& strand_service,
        ozo::tests::steady_timer_gmock& timer
    ) : ozo::tests::execution_context(executor, strand_service), timer_(std::addressof(timer)) {}
    MOCK_METHOD1(get_operation_timer, void(ozo::time_traits::time_point));
};

} // namespace

namespace ozo::detail {

template <>
struct operation_timer<io_context_mock> {
    using type = ozo::tests::steady_timer;

    template <typename TimeConstraint>
    static type get(io_context_mock& io, TimeConstraint t) {
        io.get_operation_timer(t);
        return type{io.timer_};
    }
};

} // namespace ozo::detail
namespace {

using namespace std::literals;
using namespace ::testing;

struct dispatch_cancel : Test {
    struct cancel_handle_mock {
        using self_type = cancel_handle_mock;
        MOCK_METHOD1(pq_cancel, bool(std::string& ));

        friend bool pq_cancel(self_type* self, std::string& err) {
            return self->pq_cancel(err);
        }
    } handle;
};

TEST_F(dispatch_cancel, should_return_no_error_and_empty_string_if_pq_cancel_returns_true) {
    EXPECT_CALL(handle, pq_cancel(_)).WillOnce(Return(true));
    auto [ec, msg] = ozo::impl::dispatch_cancel(&handle);
    EXPECT_FALSE(ec);
    EXPECT_TRUE(msg.empty());
}

TEST_F(dispatch_cancel, should_return_pq_cancel_failed_and_non_empty_string_if_pq_cancel_returns_false_and_sets_message) {
    EXPECT_CALL(handle, pq_cancel(_)).WillOnce(Invoke([](std::string& msg){
        msg = "error message";
        return false;
    }));
    auto [ec, msg] = ozo::impl::dispatch_cancel(&handle);
    EXPECT_EQ(ozo::error_code{ozo::error::pq_cancel_failed}, ec);
    EXPECT_FALSE(msg.empty());
}

TEST_F(dispatch_cancel, should_remove_trailing_zeroes_from_error_message) {
    EXPECT_CALL(handle, pq_cancel(_)).WillOnce(Invoke([](std::string& msg){
        msg = "error message\0\0\0\0\0\0\0\0\0\0";
        return false;
    }));
    auto [ec, msg] = ozo::impl::dispatch_cancel(&handle);
    EXPECT_TRUE(ec);
    EXPECT_EQ(msg, "error message"s);
}

TEST_F(dispatch_cancel, should_return_empty_string_from_all_zeroes) {
    EXPECT_CALL(handle, pq_cancel(_)).WillOnce(Invoke([](std::string& msg){
        msg = "\0\0\0\0\0\0\0\0\0\0";
        return false;
    }));
    auto [ec, msg] = ozo::impl::dispatch_cancel(&handle);
    EXPECT_TRUE(ec);
    EXPECT_TRUE(msg.empty());
}

struct cancel_handle_mock {
    MOCK_CONST_METHOD0(dispatch_cancel, std::tuple<ozo::error_code, std::string>());
};

struct cancel_handle {
    cancel_handle_mock* mock_ = nullptr;
    ozo::tests::executor_mock* executor_ = nullptr;

    cancel_handle(cancel_handle_mock& mock, ozo::tests::executor_mock& executor)
    : mock_(std::addressof(mock)), executor_(std::addressof(executor)) {}

    using executor_type = ozo::tests::executor;

    executor_type get_executor() const { return executor_type(*executor_);}

    friend auto dispatch_cancel(cancel_handle self) {
        return self.mock_->dispatch_cancel();
    }
};

struct initiate_async_cancel : Test {
    struct strand_service : ozo::tests::strand_executor_service_mock {
        ozo::tests::executor_gmock executor;
        ozo::tests::executor_mock& get_executor() {
            return executor;
        }
    } strand;

    StrictMock<ozo::tests::executor_gmock> executor;
    StrictMock<ozo::tests::steady_timer_gmock> timer;
    io_context_mock io{executor, strand, timer};
    StrictMock<cancel_handle_mock> cancel_handle_;
    StrictMock<ozo::tests::executor_gmock> handle_executor;
    StrictMock<ozo::tests::callback_gmock<std::string>> callback;

    ozo::impl::initiate_async_cancel initiate_async_cancel_;
};

TEST_F(initiate_async_cancel, should_post_cancel_op_into_cancel_handle_attached_executor) {
    EXPECT_CALL(handle_executor, post(_));
    initiate_async_cancel_(ozo::tests::wrap(callback), cancel_handle(cancel_handle_, handle_executor));
}


TEST_F(initiate_async_cancel, should_post_cancel_op_with_time_constraint_into_cancel_handle_attached_executor_and_wait_for_timer) {
    EXPECT_CALL(io, get_operation_timer(_));
    EXPECT_CALL(handle_executor, post(_));
    EXPECT_CALL(timer, async_wait(_));
    initiate_async_cancel_(ozo::tests::wrap(callback), cancel_handle(cancel_handle_, handle_executor), io, ozo::time_traits::time_point{});
}

TEST(on_cancel_op_timer, should_call_callback_with_asio_error_timed_out) {
    StrictMock<ozo::tests::callback_gmock<std::string>> callback;
    EXPECT_CALL(callback, call(ozo::error_code{boost::asio::error::timed_out}, _));
    ozo::impl::on_cancel_op_timer(ozo::tests::wrap(callback))(ozo::error_code{});
}

}
