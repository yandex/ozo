#pragma once

#include <ozo/impl/start_transaction.h>
#include <ozo/impl/end_transaction.h>

namespace ozo {

struct begin_op {
    template <typename T, typename TimeConstraint, typename CompletionToken>
    auto operator() (T&& provider, TimeConstraint t, CompletionToken&& token) const {
        static_assert(ConnectionProvider<T>, "provider should be a ConnectionProvider");
        static_assert(ozo::TimeConstraint<TimeConstraint>, "should model TimeConstraint concept");
        using namespace ozo::literals;
        return impl::start_transaction(
            std::forward<T>(provider),
            "BEGIN"_SQL,
            t,
            std::forward<CompletionToken>(token)
        );
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

constexpr begin_op begin;

struct commit_op {
    template <typename T, typename TimeConstraint, typename CompletionToken>
    auto operator() (impl::transaction<T>&& transaction, TimeConstraint t, CompletionToken&& token) const {
        static_assert(ozo::TimeConstraint<TimeConstraint>, "should model TimeConstraint concept");
        using namespace ozo::literals;
        return impl::end_transaction(
            std::move(transaction),
            "COMMIT"_SQL,
            t,
            std::forward<CompletionToken>(token)
        );
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
        return impl::end_transaction(
            std::move(transaction),
            "ROLLBACK"_SQL,
            t,
            std::forward<CompletionToken>(token)
        );
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
