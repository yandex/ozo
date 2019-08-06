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
 * @brief Options for retry
 *
 * These options can be used with `ozo::failover::retry_strategy`.
 * @ingroup group-failover-retry
 */
struct retry_options {
    class on_retry_tag;
    class close_connection_tag;
    class tries_tag;
    class conditions_tag;

    constexpr static hana::type<on_retry_tag> on_retry{}; //!< Set handler for retry event, may be useful for logging.
    constexpr static hana::type<close_connection_tag> close_connection{}; //!< Set close connection policy on retry, possible values `true`(default), `false`.
    constexpr static hana::type<tries_tag> tries{}; //!< Set number of tries, see `ozo::retry_strategy::tries()` for more information.
    constexpr static hana::type<conditions_tag> conditions{}; //!< Set error conditions to retry
};

/**
 * @brief Operation try representation
 *
 * Controls current try state incluiding number of tries remain, context of operation call for
 * current try.
 *
 * @tparam Options --- map of `ozo::failover::retry_options`.
 * @tparam Context --- operation call context, literally operation arguments except #CompletionToken.
 * @ingroup group-failover-retry
 */
template <typename Options, typename Context>
class basic_try {
    Context ctx_;
    std::decay_t<Options> options_;
    using op = retry_options;

public:
    /**
     * @brief Construct a new basic try object.
     *
     * @param options --- map of `ozo::failover::retry_options`.
     * @param ctx --- operation context, compartible with `ozo::failover::basic_context`
     */
    constexpr basic_try(Options options, Context ctx)
    : ctx_(std::move(ctx)), options_(std::move(options)) {
    }
    constexpr basic_try(const basic_try&) = delete;
    constexpr basic_try(basic_try&&) = default;
    constexpr basic_try& operator = (const basic_try&) = delete;
    constexpr basic_try& operator = (basic_try&&) = default;

    /**
     * @brief Update an option
     *
     * @param key --- one of `ozo::failover::retry_options`.
     * @param v --- value to set the option in
     */
    template <typename Key, typename T>
    void update_option(const Key& key, T&& v) {
        static_assert(decltype(has_option(key))::value, "option has not been found");
        options_[key] = std::forward<T>(v);
    }

    /**
     * @brief Get option value
     *
     * To get a value option should exist.
     * @param key --- one of `ozo::failover::retry_options` to get value of.
     * @return value of the option.
     */
    template <typename Key>
    constexpr decltype(auto) option(const Key& key) const {
        static_assert(decltype(has_option(key))::value, "option has not been found");
        return options_[key];
    }
    template <typename Key>
    constexpr decltype(auto) option(const Key& key) {
        static_assert(decltype(has_option(key))::value, "option has not been found");
        return options_[key];
    }

    /**
     * @brief Get option value or default value if option is not found
     *
     * @param key --- one of `ozo::failover::retry_options` to get value of.
     * @param default_ --- value to return if option has not been found.
     * @return value of the option or default value.
     */
    template <typename Key, typename T>
    constexpr decltype(auto) option(const Key& key, const T& default_) const {
        if constexpr (decltype(has_option(key))::value) {
            return options_[key];
        } else {
            return default_;
        }
    }
    template <typename Key, typename T>
    constexpr decltype(auto) option(const Key& key, const T& default_) {
        if constexpr (decltype(has_option(key))::value) {
            return options_[key];
        } else {
            return default_;
        }
    }

    /**
     * @brief Indicates option is exists
     *
     * @param key --- one of `ozo::failover::retry_options` to check if it exists.
     * @return `boost::hana::true_` --- option exists.
     * @return `boost::hana::false_` --- option does not exist.
     */
    template <typename Key>
    constexpr auto has_option(const Key& key) const {
        return hana::contains(options_, key);
    }

    /**
     * @brief Get the operation context.
     *
     * @return `boost::hana::tuple` --- operation initiation context for the try with modified time constraint;
     *                  see `ozo::retry_strategy::tries()` for details about calculations.
     */
    auto get_context() const {
        return hana::concat(
            hana::make_tuple(unwrap(ctx_).provider, time_constraint()),
            unwrap(ctx_).args
        );
    }

    /**
     * @brief Get the next try
     *
     * Return next try object for retry operation if it possible. See
     * `ozo::fallback::get_next_try()` for details.
     *
     * @param ec --- error code which should be examined for retry ability.
     * @param conn --- #Connection object which should be closed anyway.
     * @return `std::optional<basic_try>` --- initialized with basic_try object if retry is possible.
     * @return `std::nullopt` --- retry is not possible due to `ec` value or tries remain
     *                            count.
     */
    template <typename Connection>
    std::optional<basic_try> get_next_try(ozo::error_code ec, Connection&& conn) {
        std::optional<basic_try> retval;
        const auto should_close_connection = option(op::close_connection, true);
        adjust_tries_remain();
        if (can_retry(ec)) {
            option(op::on_retry, [](auto&&...){})(ec, conn);
            retval.emplace(basic_try{std::move(options_), std::move(ctx_)});
        }

        if (should_close_connection) {
            if (!is_null_recursive(conn)) {
                close_connection(conn);
            }
        }

        return retval;
    }

    /**
     * @brief Number of tries remains
     *
     * @return int --- number of tries remains to execute an operation, not less than 0.
     */
    constexpr int tries_remain() const { return option(op::tries);}

    /**
     * @brief Retry conditions for the try
     *
     * @return auto --- `boost::hana::tuple` of error conditions which have a good
     *                             chance to be solved by a retry.
     */
    constexpr decltype(auto) get_conditions() const {
        return option(op::conditions, no_conditions_);
    }

private:
    void adjust_tries_remain() {
        update_option(op::tries, std::max(0, tries_remain() - 1));
    }

    auto time_constraint() const {
        return detail::get_try_time_constraint(unwrap(ctx_).time_constraint, tries_remain());
    }

    bool can_retry([[maybe_unused]] error_code ec) const {
        if(tries_remain() < 1) {
            return false;
        }

        if constexpr (decltype(hana::is_empty(get_conditions()))::value) {
            return true;
        } else {
            const auto f = [ec](bool v, auto errc) { return v || (ec == errc); };
            return boost::hana::fold(get_conditions(), false, f);
        }
    }
    constexpr static const auto no_conditions_ = hana::make_tuple();
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
template <typename Options = decltype(hana::make_map())>
class retry_strategy {
    std::decay_t<Options> options_;
    using op = retry_options;

    template <typename OtherOptions>
    constexpr static auto rebind(OtherOptions&& options) {
        return retry_strategy<std::decay_t<OtherOptions>>(std::forward<OtherOptions>(options));
    }

public:
    /**
     * @brief Construct a new retry strategy object
     *
     * @param options --- `boost::hana::map` of `ozo::failover::retry_options` and values.
     */
    retry_strategy(Options options = Options{}) : options_(std::move(options)) {}

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
    auto get_first_try(const Operation&, const Allocator&,
            ConnectionProvider&& provider, TimeConstraint t, Args&& ...args) const {

        static_assert(decltype(has(op::tries))::value, "number of tries should be specified");

        return basic_try {
            std::move(options_),
            basic_context{
                std::forward<ConnectionProvider>(provider),
                ozo::deadline(t),
                std::forward<Args>(args)...
            }
        };
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
    constexpr decltype(auto) tries(int n) const & { return set(op::tries, n);}
    constexpr decltype(auto) tries(int n) && { return std::move(*this).set(op::tries, n);}

    /**
     * @brief Number of maximum tries count are setted with `ozo::retry_strategy::tries()`
     *
     * @return int --- number of operation tries
     */
    constexpr int get_tries() const { return get(op::tries); }

    /**
     * @brief Retry error conditions of the strategy
     *
     * @return `boost::hana::tuple` --- tuple of retryable error conditions setted for this strategy.
     */
    constexpr auto get_conditions() const { return get(op::conditions); }

    /**
     * @brief Set option for the strategy
     *
     * Insert or update option with given value.
     *
     * @param key --- one of `ozo::failover::retry_options`.
     * @param value --- value of the option to set option in.
     * @return `retry_strategy` object.
     */
    template <typename Option, typename T>
    constexpr decltype(auto) set(const Option& key, T&& value) && {
        if constexpr (decltype(has(key))::value) {
            options_[key] = std::forward<T>(value);
            return std::move(*this);
        } else {
            auto options = hana::insert(std::move(options_), hana::make_pair(key, std::forward<T>(value)));
            return rebind(std::move(options));
        }
    }

    template <typename Option, typename T>
    constexpr decltype(auto) set(const Option& key, T&& value) const & {
        retry_strategy copy = *this;
        return static_cast<retry_strategy&&>(copy).set(key, std::forward<T>(value));
    }

    /**
     * @brief Indicates option has been set
     *
     * @param key --- one of `ozo::failover::retry_options` to check.
     * @return `boost::hana::true_` --- option has been set.
     * @return `boost::hana::false_` --- option has not been set.
     */
    template <typename Option>
    constexpr auto has(const Option& key) const {
        return hana::contains(options_, key);
    }

    /**
     * @brief Get option value
     *
     * To get a value option should be set first.
     * @param key --- one of `ozo::failover::retry_options` to get value of.
     * @return value of the option.
     */
    template <typename Option>
    constexpr decltype(auto) get(const Option& key) const {
        static_assert(decltype(has(key))::value, "the option has not been set");
        return options_[key];
    }

    /**
     * @brief Sintactic sugar for `ozo::retry_strategy::tries()`
     *
     * @param n --- number of tries
     * @return `retry_strategy` specialization object
     */
    constexpr decltype(auto) operator * (int n) const & { return tries(n); }
    constexpr decltype(auto) operator * (int n) && { return std::move(*this).tries(n); }
};

template <typename ...Ts>
constexpr decltype(auto) operator * (int n, const retry_strategy<Ts...>& rs) { return rs * n;}
template <typename ...Ts>
constexpr decltype(auto) operator * (int n, retry_strategy<Ts...>&& rs) { return std::move(rs) * n;}

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
auto retry = failover::retry(errc::connection_error, errc::timeout)*3;
ozo::request[retry](pool, query, .5s, out, yield);
 * @endcode
 *
 * @sa `ozo::failover::retry_strategy`
 * @ingroup group-failover-retry
 */
template <typename ...ErrorConditions>
constexpr auto retry(ErrorConditions ...ec) {
    if constexpr (sizeof...(ec) != 0) {
        return retry_strategy{}.set(retry_options::conditions, hana::make_tuple(ec...));
    } else {
        return retry_strategy{};
    }
}

} // namespace ozo::failover

namespace ozo {

template <typename ...Ts, typename Op>
struct construct_initiator_impl<failover::retry_strategy<Ts...>, Op>
: failover::construct_initiator_impl {};

} // namespace ozo
