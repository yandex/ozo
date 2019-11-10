#pragma once

#include <ozo/core/options.h>
#include <ozo/detail/begin_statement_builder.h>
#include <ozo/impl/async_start_transaction.h>
#include <ozo/impl/async_end_transaction.h>
#include <ozo/transaction_status.h>

namespace ozo {

template <typename Initiator, typename Options = decltype(make_options())>
struct begin_op : base_async_operation <begin_op<Initiator, Options>, Initiator> {
    using base = typename begin_op::base;
    Options options_;

    constexpr explicit begin_op(Initiator initiator = {}, Options options = {}) : base(initiator), options_(options) {}

    template <typename T, typename TimeConstraint, typename CompletionToken>
    auto operator() (T&& provider, TimeConstraint t, CompletionToken&& token) const {
        static_assert(ConnectionProvider<T>, "provider should be a ConnectionProvider");
        static_assert(ozo::TimeConstraint<TimeConstraint>, "should model TimeConstraint concept");
        using namespace ozo::literals;
        return async_initiate<CompletionToken, handler_signature<impl::transaction<connection_type<T>, Options>>>(
            get_operation_initiator(*this), token,
            std::forward<T>(provider), options_, detail::begin_statement_builder::build(options_), t);
    }

    template <typename T, typename CompletionToken>
    auto operator() (T&& provider, CompletionToken&& token) const {
        return (*this)(
            std::forward<T>(provider),
            none,
            std::forward<CompletionToken>(token)
        );
    }

    template <typename OtherOptions>
    constexpr auto with_transaction_options(const OtherOptions& options) const {
        return begin_op<Initiator, OtherOptions>{get_operation_initiator(*this), options};
    }

    template <typename OtherInitiator>
    constexpr auto rebind_initiator(const OtherInitiator& other) const {
        return begin_op<OtherInitiator, Options>{other, options_};
    }
};

constexpr begin_op<impl::initiate_async_start_transaction> begin;

struct commit_op {
    template <typename T, typename Options, typename TimeConstraint, typename CompletionToken>
    auto operator() (impl::transaction<T, Options>&& transaction, TimeConstraint t, CompletionToken&& token) const {
        static_assert(ozo::TimeConstraint<TimeConstraint>, "should model TimeConstraint concept");
        using namespace ozo::literals;
        return async_initiate<CompletionToken, handler_signature<T>>(
            impl::initiate_async_end_transaction{}, token,
            std::move(transaction), "COMMIT"_SQL, t);
    }

    template <typename... Ts, typename CompletionToken>
    auto operator() (impl::transaction<Ts...>&& transaction, CompletionToken&& token) const {
        return (*this)(
            std::move(transaction),
            none,
            std::forward<CompletionToken>(token)
        );
    }
};

constexpr commit_op commit;

struct rollback_op {
    template <typename T, typename Options, typename TimeConstraint, typename CompletionToken>
    auto operator() (impl::transaction<T, Options>&& transaction, TimeConstraint t, CompletionToken&& token) const {
        static_assert(ozo::TimeConstraint<TimeConstraint>, "should model TimeConstraint concept");
        using namespace ozo::literals;
        return async_initiate<CompletionToken, handler_signature<T>>(
            impl::initiate_async_end_transaction{}, token,
            std::move(transaction), "ROLLBACK"_SQL, t);
    }

    template <typename... Ts, typename CompletionToken>
    auto operator() (impl::transaction<Ts...>&& transaction, CompletionToken&& token) const {
        return (*this)(
            std::move(transaction),
            none,
            std::forward<CompletionToken>(token)
        );
    }
};

constexpr rollback_op rollback;

/**
 * @ingroup group-connection-functions
 * @brief Retrieve's a transactions isolation level
 *
 * The isolation level can either be an instance of one of the child types of ozo::isolation_level or
 * `ozo::none` if no level was explicitly set.
 *
 * @param transaction transaction to get the isolation level from
 * @return sub type of ozo::isolation_level --- if the transaction isolation level has been set
 * @return ozo::none --- if no isolation level has been set
 */
template<typename... Ts>
constexpr auto get_transaction_isolation_level(const impl::transaction<Ts...>& transaction) {
    return get_option(transaction.options(), transaction_options::isolation_level, none);
}

/**
 * @ingroup group-connection-functions
 * @brief Retrieve's a transactions transaction mode
 *
 * The mode can either be an instance of one of the child types of ozo::transaction_mode or
 * ozo::none if mode was set explicitly set.
 *
 * @param transaction transaction to get the transaction mode from
 * @return sub type of ozo::transaction_mode --- if the transaction mode has been set
 * @return ozo::none --- if no transaction mode has been specified
 */
template<typename... Ts>
constexpr auto get_transaction_mode(const impl::transaction<Ts...>& transaction) {
    return get_option(transaction.options(), transaction_options::mode, none);
}

/**
 * @ingroup group-connection-functions
 * @brief Retrieve's a transactions deferrability
 *
 * Deferrability is usually indicated using an instance of ozo::deferrable_mode, but can also
 * be any other type with a compile-time static ::value convertible to bool.
 * `ozo::none` indicates no explicit deferrability was set.
 *
 * @param transaction transaction to get the deferrability from
 * @return ozo::deferrable_mode (or any bool-convertible std::integral_constant) --- if the deferrability has been set
 * @return ozo::none --- if no deferrability has been set
 */
template<typename... Ts>
constexpr auto get_transaction_deferrability(const impl::transaction<Ts...>& transaction) {
    return get_option(transaction.options(), transaction_options::deferrability, none);
}

} // namespace ozo
