#pragma once

#include <ozo/asio.h>
#include <ozo/connection.h>

namespace ozo {

template <typename Base, typename ... Args>
class connector {
public:
    static_assert(ConnectionSource<Base>, "is not a ConnectionSource");

    using source_type = Base;
    using connection_type = typename source_type::connection_type;

    connector(Base& base, io_context& io, std::tuple<Args ...>&& args)
            : base(base), io(io), args(std::move(args)) {
    }

    template <typename Handler>
    void async_get_connection(Handler&& handler) const& {
        std::apply(
            [&] (const auto& ... parameters) {
                base(io, std::forward<Handler>(handler), parameters ...);
            },
            args
        );
    }

    template <typename Handler>
    void async_get_connection(Handler&& handler) & {
        std::apply(
            [&] (auto& ... parameters) {
                base(io, std::forward<Handler>(handler), parameters ...);
            },
            args
        );
    }

    template <typename Handler>
    void async_get_connection(Handler&& handler) && {
        std::apply(
            [&] (auto&& ... parameters) {
                base(io, std::forward<Handler>(handler), parameters ...);
            },
            std::move(args)
        );
    }

private:
    Base& base;
    io_context& io;
    std::tuple<Args ...> args;
};

template <class Base, class ... Args>
auto make_connector(Base& base, io_context& io, Args&& ... args) {
    return connector<std::decay_t<Base>, std::decay_t<Args> ...>(
        base,
        io,
        std::make_tuple(std::forward<Args>(args) ...)
    );
}

} // namespace ozo
