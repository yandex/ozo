#pragma once

#include <ozo/connection.h>
#include <ozo/transaction_options.h>

namespace ozo::impl {

template <typename T, typename Options>
class transaction {
    friend unwrap_impl<transaction>;

public:
    static_assert(Connection<T>, "T is not a Connection");

    transaction() = default;

    transaction(T connection, Options options)
            : impl(is_null(connection) ? nullptr : std::make_shared<impl_type>(std::move(connection))), options_(std::move(options)) {}

    ~transaction() {
        const auto c = std::move(impl);
        if (c.use_count() == 1 && c->connection) {
            close_connection(*c->connection);
        }
    }

    void take_connection(T& out) {
        out = std::move(*impl->connection);
        impl->connection = OZO_STD_OPTIONAL<T>{};
        impl.reset();
    }

    bool has_connection() const {
        return impl != nullptr && impl->has_connection();
    }

    operator bool() const {
        return has_connection();
    }

    constexpr Options& options() { return options_; }
    constexpr const Options& options() const { return options_; }

private:
    struct impl_type {
        OZO_STD_OPTIONAL<T> connection;

        impl_type(T&& connection) : connection(std::move(connection)) {}

        bool has_connection() const {
            return connection.has_value() && !is_null(*connection);
        }
    };

    std::shared_ptr<impl_type> impl;
    std::decay_t<Options> options_;
};

template <typename T, typename Options, typename = Require<Connection<T>>>
auto make_transaction(T&& conn, Options&& options) {
    return transaction<std::decay_t<T>, std::decay_t<Options>> {std::forward<T>(conn), std::forward<Options>(options)};
}

} // namespace ozo::impl

namespace ozo {

template <typename... Ts>
struct unwrap_impl<impl::transaction<Ts...>> {
    template <typename Transaction>
    static constexpr decltype(auto) apply(Transaction&& self) noexcept {
        return unwrap(*self.impl->connection);
    }
};

template <typename... Ts>
struct is_nullable<impl::transaction<Ts...>> : std::true_type {};

} // namespace ozo
