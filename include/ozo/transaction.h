#pragma once

#include <ozo/impl/start_transaction.h>
#include <ozo/impl/end_transaction.h>

namespace ozo {

struct begin_op {
    template <typename T, typename CompletionToken>
    auto operator() (T&& provider, const time_traits::duration& timeout, CompletionToken&& token) const {
        static_assert(ConnectionProvider<T>, "provider should be a ConnectionProvider");
        using namespace ozo::literals;
        return impl::start_transaction(
            std::forward<T>(provider),
            "BEGIN"_SQL,
            timeout,
            std::forward<CompletionToken>(token)
        );
    }

    template <typename T, typename CompletionToken>
    auto operator() (T&& provider, CompletionToken&& token) const {
        return (*this)(
            std::forward<T>(provider),
            time_traits::duration::max(),
            std::forward<CompletionToken>(token)
        );
    }
};

constexpr begin_op begin;

struct commit_op {
    template <typename T, typename CompletionToken>
    auto operator() (impl::transaction<T>&& transaction, const time_traits::duration& timeout, CompletionToken&& token) const {
        using namespace ozo::literals;
        return impl::end_transaction(
            std::move(transaction),
            "COMMIT"_SQL,
            timeout,
            std::forward<CompletionToken>(token)
        );
    }

    template <typename T, typename CompletionToken>
    auto operator() (impl::transaction<T>&& transaction, CompletionToken&& token) const {
        return (*this)(
            std::move(transaction),
            time_traits::duration::max(),
            std::forward<CompletionToken>(token)
        );
    }
};

constexpr commit_op commit;

struct rollback_op {
    template <typename T, typename CompletionToken>
    auto operator() (impl::transaction<T>&& transaction, const time_traits::duration& timeout, CompletionToken&& token) const {
        using namespace ozo::literals;
        return impl::end_transaction(
            std::move(transaction),
            "ROLLBACK"_SQL,
            timeout,
            std::forward<CompletionToken>(token)
        );
    }

    template <typename T, typename CompletionToken>
    auto operator() (impl::transaction<T>&& transaction, CompletionToken&& token) const {
        return (*this)(
            std::move(transaction),
            time_traits::duration::max(),
            std::forward<CompletionToken>(token)
        );
    }
};

constexpr rollback_op rollback;

} // namespace ozo
