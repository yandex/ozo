#pragma once

#include <ozo/connection.h>

namespace ozo::impl {

template <typename T>
class transaction {
public:
    static_assert(Connection<T>, "T is not a Connection");

    transaction() = default;

    transaction(T connection)
            : impl(std::make_shared<impl_type>(std::move(connection))) {}

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

    friend auto& unwrap_connection(transaction& self) noexcept {
        return unwrap_connection(*self.impl->connection);
    }

    friend const auto& unwrap_connection(const transaction& self) noexcept {
        return unwrap_connection(*self.impl->connection);
    }

private:
    struct impl_type {
        __OZO_STD_OPTIONAL<T> connection;

        impl_type(T&& connection) : connection(std::move(connection)) {}
    };

    std::shared_ptr<impl_type> impl;
};

template <typename T, typename = Require<Connection<T>>>
auto make_transaction(T&& conn) {
    return transaction<std::decay_t<T>> {std::forward<T>(conn)};
}

} // namespace ozo::impl
