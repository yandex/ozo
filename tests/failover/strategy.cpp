#include <ozo/failover/strategy.h>

#include "../test_error.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace {
struct connection_mock {};

struct provider_mock {};

struct try_mock {
    MOCK_CONST_METHOD2(get_next_try, try_mock*(ozo::error_code, connection_mock*));
    using context = boost::hana::tuple<provider_mock*, ozo::time_traits::duration, int, std::string>;
    MOCK_CONST_METHOD0(get_context, context());
};

struct handler_mock {
    struct wrapper {
        handler_mock const* mock_ = nullptr;
        void operator () (ozo::error_code ec, connection_mock* conn) const {
            mock_->call(ec, conn);
        };
        bool operator == (const wrapper& rhs) const {
            return mock_ == rhs.mock_;
        }
    };
    MOCK_CONST_METHOD2(call, void(ozo::error_code, connection_mock*));
    auto f() const { return wrapper{this}; }
};

struct operation {
    using handler_type = ozo::failover::detail::continuation<operation, try_mock*, decltype(std::declval<handler_mock>().f())>;
    struct initiator_mock {
        MOCK_CONST_METHOD5(call, void(
            handler_type,
            provider_mock* provider,
            ozo::time_traits::duration t,
            int arg1,
            std::string arg2
        ));
    };
    initiator_mock * mock = nullptr;

    struct initiator_type {
        initiator_mock * mock = nullptr;
        initiator_type(initiator_mock* mock) : mock(mock) {}

        void operator() (handler_type h, provider_mock* provider, ozo::time_traits::duration t, int arg1, std::string arg2) {
            if (!mock) {
                throw std::invalid_argument("mock should not be nullptr");
            }
            mock->call(std::move(h), provider, t, arg1, arg2);
        }
    };

    initiator_type get_initiator() const {
        return {mock};
    }

    bool operator == (const operation& rhs) const {
        return mock == rhs.mock;
    }
};



} // namespace

namespace ozo {
// Some cheats about connection_mock which is not a connection
// at all.
template <>
struct is_connection<connection_mock*> : std::true_type {};

template <>
struct is_nullable<connection_mock*> : std::true_type {};

template <>
struct is_nullable<try_mock*> : std::true_type {};

template <>
struct unwrap_impl<try_mock*> : ozo::detail::functional::dereference {};

namespace failover::detail {

template <typename ...Ts>
inline bool operator == (const continuation<Ts...>& lhs, const continuation<Ts...>& rhs) {
    return std::tie(lhs.op_, lhs.try_, lhs.handler_) == std::tie(rhs.op_, rhs.try_, rhs.handler_);
}

} // namespace failover::detail
} // namespace ozo

namespace {

using namespace testing;
using namespace std::literals;
using namespace std::chrono_literals;

struct continuation : Test {
    handler_mock handler;
    operation::initiator_mock initiator;
    operation op{std::addressof(initiator)};
    try_mock a_try;
    connection_mock conn;
    provider_mock provider;
};

TEST_F(continuation, should_call_handler_if_called_with_no_error) {
    EXPECT_CALL(handler, call(ozo::error_code{}, std::addressof(conn)));

    auto continuation = ozo::failover::detail::continuation{op, std::addressof(a_try), handler.f()};
    continuation(ozo::error_code{}, std::addressof(conn));
}

TEST_F(continuation, should_call_handler_if_called_with_error_and_no_next_try) {
    InSequence s;
    EXPECT_CALL(a_try, get_next_try(ozo::error_code{ozo::tests::error::error}, std::addressof(conn)))
        .WillOnce(Return(nullptr));
    EXPECT_CALL(handler, call(ozo::error_code{ozo::tests::error::error}, std::addressof(conn)));

    auto continuation = ozo::failover::detail::continuation{op, std::addressof(a_try), handler.f()};
    continuation(ozo::error_code{ozo::tests::error::error}, std::addressof(conn));
}

TEST_F(continuation, should_initiate_operation_with_context_and_continuation_if_called_with_error_and_has_next_try) {
    InSequence s;
    EXPECT_CALL(a_try, get_next_try(ozo::error_code{ozo::tests::error::error}, std::addressof(conn)))
        .WillOnce(Return(std::addressof(a_try)));
    EXPECT_CALL(a_try, get_context())
        .WillOnce(Return(boost::hana::make_tuple(std::addressof(provider), 3s, 42, "some string"s)));
    EXPECT_CALL(initiator, call(
        ozo::failover::detail::continuation{op, std::addressof(a_try), handler.f()},
        std::addressof(provider), ozo::time_traits::duration{3s}, 42, "some string"s));

    auto continuation = ozo::failover::detail::continuation{op, std::addressof(a_try), handler.f()};
    continuation(ozo::error_code{ozo::tests::error::error}, std::addressof(conn));
}

struct strategy_mock {
    MOCK_CONST_METHOD6(get_first_try, try_mock*(
        const operation&,
        const std::allocator<char>& ,
        provider_mock* provider,
        ozo::time_traits::duration t,
        int arg1,
        std::string arg2));
};

struct strategy_impl {
    strategy_mock* mock_ = nullptr;
    template <typename ...Ts>
    auto get_first_try(Ts&&... vs) const {
        return mock_->get_first_try(std::forward<Ts>(vs)...);
    }
};

TEST(operation_initiator, should_call_get_first_try_and_initiate_operation_via_its_initiator) {
    handler_mock handler;
    strategy_mock strategy;
    operation::initiator_mock initiator;
    operation op{std::addressof(initiator)};
    provider_mock provider;
    try_mock a_try;

    EXPECT_CALL(strategy, get_first_try(op, _,
        std::addressof(provider),
        ozo::time_traits::duration{3s},
        42,
        "some string"s))
        .WillOnce(Return(std::addressof(a_try)));

    EXPECT_CALL(a_try, get_context())
        .WillOnce(Return(boost::hana::make_tuple(std::addressof(provider), 3s, 42, "some string"s)));

    EXPECT_CALL(initiator, call(
        ozo::failover::detail::continuation{op, std::addressof(a_try), handler.f()},
        std::addressof(provider), ozo::time_traits::duration{3s}, 42, "some string"s));

    ozo::failover::detail::operation_initiator(strategy_impl{std::addressof(strategy)}, op)(
        handler.f(),
        std::addressof(provider),
        3s, 42, "some string"s);
}

} // namespace
