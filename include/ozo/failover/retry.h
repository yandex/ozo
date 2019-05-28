#pragma once

#include <ozo/failover/strategy.h>
#include <boost/hana/is_empty.hpp>
#include <boost/hana/tuple.hpp>
#include <boost/hana/fold.hpp>
#include <boost/hana/concat.hpp>

/**
 * @defgroup group-failover-retry Retry
 * @ingroup group-failover
 * @brief Failover by simple retry operation
 */

namespace ozo::failover {

namespace detail {

template <typename TimeConstraint, typename Now = decltype(time_traits::now)>
inline auto get_try_time_constraint(TimeConstraint t, int n_tries, [[maybe_unused]] Now now = time_traits::now) {
    if constexpr (t == none) {
        return none;
    } else if constexpr (std::is_same_v<TimeConstraint, time_traits::time_point>) {
        return n_tries > 0 ? time_left(t, now()) / n_tries : time_traits::duration{0};
    } else {
        return n_tries > 0 ? t / n_tries : time_traits::duration{0};
    }
}

template <typename ...ErrorConditions>
constexpr auto make_errcs_tuple(ErrorConditions ...errcs) {
    return hana::make_tuple(errcs...);
}

} // namespace detail

/**
 * @brief Operation try representation
 *
 * Controls current try state incluiding number of tries remain, context of operation call for
 * current try.
 *
 * @tparam ErrorConditions --- error conditions to match on for retry, should model #HanaSequence
 * @tparam Context --- operation call context, literally operation arguments except #CompletionToken
 * @ingroup group-failover-retry
 */
template <typename ErrorConditions, typename Context>
class basic_try {
public:
    static_assert(HanaSequence<ErrorConditions>, "ErrorConditions should models HanaSequence");

    /**
     * @brief Construct a new basic try object.
     *
     * @param n_tries --- number of tries allowed, should not be less than 0.
     * @param errcs --- retry conditions should model #HanaSequence for `ozo::error_code` and
     *                  `ozo::error_condition` comparable types
     * @param ctx --- operation context, compartible with `ozo::failover::basic_context`
     */
    basic_try(int n_tries, ErrorConditions errcs, Context ctx)
    : ctx_(std::move(ctx)), errcs_(errcs), tries_remain_(n_tries) {
        if (n_tries < 0) {
            throw std::invalid_argument("number of tries should not be less than 0");
        }
    };

    /**
     * @brief Get the operation context.
     *
     * @return auto --- operation context for the try with modified time constraint;
     *                  see `ozo::retry_strategy::tries()` for details about calculations.
     */
    auto get_context() const {
        return hana::concat(
            hana::make_tuple(unwrap(ctx_).provider, time_constraint()),
            unwrap(ctx_).args
        );
    }

    /**
     * @brief Number of tries remains
     *
     * @return int --- number of tries remains to execute an operation, not less than 0.
     */
    int tries_remain() const { return tries_remain_;}

    /**
     * @brief Retry conditions for the try
     *
     * @return ErrorConditions --- #HanaSequence of retry conditions which have a
     *                             chance to be solved by this try execution.
     */
    ErrorConditions get_conditions() const { return errcs_; }

    /**
     * @brief Get the next try
     *
     * Return next try object for retry operation if it possible. See
     * `ozo::fallback::get_next_try()` for details.
     *
     * @param ec --- error code which should be examined for retry ability.
     * @param conn --- #Connection object which should be closed anyway.
     * @return `std::optional` --- initialized with basic_try object if retry is possible.
     * @return `std::nullopt` --- retry is not possible due to `ec` value or tries remain
     *                            count.
     */
    template <typename Connection>
    std::optional<basic_try> get_next_try(error_code ec, Connection&& conn) const {
        if (!is_null_recursive(conn)) {
            close_connection(conn);
        }
        if (tries_remain()) {
            auto retval = basic_try(tries_remain() - 1, get_conditions(), ctx_);
            if(retval.can_retry(ec)) {
                return retval;
            }
        }
        return std::nullopt;
    }

private:
    auto time_constraint() const {
        return detail::get_try_time_constraint(unwrap(ctx_).time_constraint, tries_remain());
    }

    bool can_retry([[maybe_unused]] error_code ec) const {
        if(!tries_remain()) {
            return false;
        }

        if constexpr (decltype(hana::is_empty(get_conditions()))::value) {
            return true;
        } else {
            const auto f = [ec](bool v, auto errc) { return v || (ec == errc); };
            return boost::hana::fold(get_conditions(), false, f);
        }
    }

    Context ctx_;
    ErrorConditions errcs_;
    int tries_remain_ = 1;
};

/**
 * @brief Retry strategy
 *
 * Retry strategy is a factory for `ozo::fallback::basic_try` object.
 *
 * @tparam ErrorConditions --- #HanaSequence of retry conditions,
 *                             see `ozo::failover::retry` for details.
 * @ingroup group-failover-retry
 */
template <typename ErrorConditions>
class retry_strategy {
public:
    constexpr retry_strategy(ErrorConditions errcs) : errcs_(std::move(errcs)) {}

    /**
     * @brief Get the first try object
     *
     * Default implementation for `ozo::fallback::get_first_try()` failover
     * strategy interface function.
     *
     * @param alloc --- allocator which should be used for try context.
     * @param args --- operation arguments.
     * @return basic_try --- first try object.
     */
    template <typename Operation, typename Allocator, typename ConnectionProvider,
        typename TimeConstraint, typename ...Args>
    auto get_first_try(const Operation&, const Allocator& alloc,
            ConnectionProvider&& provider, TimeConstraint t, Args&& ...args) const {
        auto ctx = detail::allocate_shared<basic_context>(
            alloc, std::forward<ConnectionProvider>(provider), ozo::deadline(t),
            std::forward<Args>(args)...
        );
        return basic_try(get_tries(), get_conditions(), std::move(ctx));
    }

    /**
     * @brief Specify number of tries
     *
     * Specify number of tries for an operation.
     *
     * @param n --- number of tries
     * @return `retry_strategy` specialization object
     *
     * If operation has time constraint `T` each try would have its own
     * time constraint according to the rule, there t<small>i</small> --- actual time of i<small>th</small> try:
     * | Try number | Time constraint |
     * |-----|------------------------------|
     * | 1   | `T`/`n`                      |
     * | 2   | (`T` - t<small>1</small>) / (n - 1) |
     * | 3   | (`T` - (t<small>1</small> + t<small>2</small>)) / (n - 2) |
     * | ... | |
     * | N   | (`T` - (t<small>1</small> + t<small>2</small> + ... + t<small>n-1</small>))) |
     *
     * ###Example
     *
     * Retry on a network problem and operation timed-out no more than 3 tries.
     * Each try has own time duration constraint calculated as:
     * - for the 1st try: 0,5/3 sec.
     * - for the 2nd try: (0,5 - t<small>1</small>) / 2 >= 0,5/3 sec.
     * - for the 3rd try: 0,5 - (t<small>1</small> + t<small>2</small>) >= 0,5/3 sec.
     *
     * @code
    auto retry = failover::retry(errc::network, errc::timeout);
    ozo::request[retry*3](pool, query, .5s, out, yield);
     * @endcode
     */
    constexpr auto tries(int n) const {
        if (n < 0) {
            throw std::invalid_argument(
                "ozo::failover::retry_strategy::tries() argument "
                "should not be less than 0");
        }
        auto retval = *this;
        retval.tries_ = n;
        return retval;
    }

    /**
     * @brief Sintactic sugar for `ozo::retry_strategy::tries()`
     *
     * @param n --- number of tries
     * @return `retry_strategy` specialization object
     */
    constexpr auto operator * (int n) const { return tries(n); }

    /**
     * @brief number of operation tries setted with `ozo::retry_strategy::tries()`
     *
     * @return int --- number of operation tries
     */
    constexpr int get_tries() const { return tries_; }

    /**
     * @brief Retry conditions for the strategy
     *
     * @return ErrorConditions --- retry conditions setted for this strategy.
     */
    constexpr ErrorConditions get_conditions() const { return errcs_; }

private:
    int tries_ = 1;
    ErrorConditions errcs_;
};

template <typename ErrorConditions>
inline auto operator * (int n, retry_strategy<ErrorConditions> rs) {
    return rs * n;
}

/**
 * Retry on specified error conditions.
 *
 * @param ec --- variadic of error conditions to retry
 * @return `ozo::failover::retry_strategy` specialization.
 *
 * ###Example
 *
 * Retry on a network problem and operation timed-out no more than 3 tries.
 * Each try has own time constraint calculated from total operation time constraint.
 * See `ozo::failover::retry_strategy::tries()` for time constraint details.
 *
 * @code
auto retry = failover::retry(errc::network, errc::timeout)*3;
ozo::request[retry](pool, query, .5s, out, yield);
 * @endcode
 *
 * @sa `ozo::failover::retry_strategy`
 * @ingroup group-failover-retry
 */
template <typename ...ErrorConditions>
constexpr auto retry(ErrorConditions ...ec) {
    return retry_strategy(detail::make_errcs_tuple(ec...));
}

} // namespace ozo::failover

namespace ozo {

template <typename ...Ts, typename Op>
struct construct_initiator_impl<failover::retry_strategy<Ts...>, Op>
: failover::construct_initiator_impl {};

} // namespace ozo
