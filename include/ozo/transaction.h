#pragma once

#include <ozo/impl/async_start_transaction.h>
#include <ozo/impl/async_end_transaction.h>

namespace ozo {

template <typename Initiator>
struct begin_op : base_async_operation <begin_op, Initiator> {
    using base = typename begin_op::base;
    using base::base;
    template <typename T, typename TimeConstraint, typename CompletionToken>
    auto operator() (T&& provider, TimeConstraint t, CompletionToken&& token) const {
        static_assert(ConnectionProvider<T>, "provider should be a ConnectionProvider");
        static_assert(ozo::TimeConstraint<TimeConstraint>, "should model TimeConstraint concept");
        using namespace ozo::literals;
        return async_initiate<CompletionToken, handler_signature<impl::transaction<connection_type<T>>>>(
            get_operation_initiator(*this), token,
            std::forward<T>(provider), "BEGIN"_SQL, t);
    }

    template <typename T, typename CompletionToken>
    auto operator() (T&& provider, CompletionToken&& token) const {
        return (*this)(
            std::forward<T>(provider),
            none,
            std::forward<CompletionToken>(token)
        );
    }
};

constexpr begin_op<impl::initiate_async_start_transaction> begin;

struct commit_op {
    template <typename T, typename TimeConstraint, typename CompletionToken>
    auto operator() (impl::transaction<T>&& transaction, TimeConstraint t, CompletionToken&& token) const {
        static_assert(ozo::TimeConstraint<TimeConstraint>, "should model TimeConstraint concept");
        using namespace ozo::literals;
        return async_initiate<CompletionToken, handler_signature<T>>(
            impl::initiate_async_end_transaction{}, token,
            std::move(transaction), "COMMIT"_SQL, t);
    }

    template <typename T, typename CompletionToken>
    auto operator() (impl::transaction<T>&& transaction, CompletionToken&& token) const {
        return (*this)(
            std::move(transaction),
            none,
            std::forward<CompletionToken>(token)
        );
    }
};

constexpr commit_op commit;

struct rollback_op {
    template <typename T, typename TimeConstraint, typename CompletionToken>
    auto operator() (impl::transaction<T>&& transaction, TimeConstraint t, CompletionToken&& token) const {
        static_assert(ozo::TimeConstraint<TimeConstraint>, "should model TimeConstraint concept");
        using namespace ozo::literals;
        return async_initiate<CompletionToken, handler_signature<T>>(
            impl::initiate_async_end_transaction{}, token,
            std::move(transaction), "ROLLBACK"_SQL, t);
    }

    template <typename T, typename CompletionToken>
    auto operator() (impl::transaction<T>&& transaction, CompletionToken&& token) const {
        return (*this)(
            std::move(transaction),
            none,
            std::forward<CompletionToken>(token)
        );
    }
};

constexpr rollback_op rollback;

} // namespace ozo
