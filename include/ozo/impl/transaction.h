#pragma once

#include <ozo/connection.h>

namespace ozo::impl {

template <typename T>
class transaction {
    friend unwrap_impl<transaction>;

public:
    static_assert(Connection<T>, "T is not a Connection");

    transaction() = default;

    transaction(T connection)
            : impl(is_null(connection) ? nullptr : std::make_shared<impl_type>(std::move(connection))) {}

    ~transaction() {
        const auto c = std::move(impl);
        if (c.use_count() == 1 && c->connection) {
            close_connection(*c->connection);
        }
    }

    void take_connection(T& out) {
        out = std::move(*impl->connection);
        impl->connection = __OZO_STD_OPTIONAL<T>{};
        impl.reset();
    }

    bool has_connection() const {
        return impl != nullptr && impl->has_connection();
    }

    operator bool() const {
        return has_connection();
    }

private:
    struct impl_type {
        __OZO_STD_OPTIONAL<T> connection;

        impl_type(T&& connection) : connection(std::move(connection)) {}

        bool has_connection() const {
            return connection.has_value() && !is_null(*connection);
        }
    };

    std::shared_ptr<impl_type> impl;
};

template <typename T, typename = Require<Connection<T>>>
auto make_transaction(T&& conn) {
    return transaction<std::decay_t<T>> {std::forward<T>(conn)};
}

} // namespace ozo::impl

namespace ozo {

template <typename T>
struct unwrap_impl<impl::transaction<T>> {
    template <typename Transaction>
    static constexpr decltype(auto) apply(Transaction&& self) noexcept {
        return unwrap(*self.impl->connection);
    }
};

template <typename T>
struct is_nullable<impl::transaction<T>> : std::true_type {};

} // namespace ozo
