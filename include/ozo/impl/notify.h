#pragma once

#include <ozo/notify.h>
#include <ozo/core/nullable.h>

namespace ozo {

template <typename Connection>
notification get_notification(Connection& conn) {
    static_assert(ozo::Connection<Connection>, "conn should model Connection");
    return hana::to<ozo::pg::shared_notify>(
        ozo::pg::make_safe(PQnotifies(ozo::get_native_handle(conn))));
}

namespace detail {

template <typename Connection, typename Handler>
struct async_wait_notification_op {
    Connection conn_;
    Handler handler_;

    async_wait_notification_op(Connection conn, Handler handler)
    : conn_(std::move(conn)), handler_(std::move(handler)) {}

    void perform() {
        conn_.async_wait_read(std::move(*this));
    }

    void operator() (error_code ec, int = 0) {
        if (!ec) {
            PQconsumeInput(ozo::get_native_handle(conn));
        }
        handler_(ec, std::move(conn));
    }

    using executor_type = asio::associated_executor_t<Handler>;

    executor_type get_executor() const noexcept {
        return asio::get_associated_executor(handler_);
    }

    using allocator_type = asio::associated_allocator_t<Handler>;

    allocator_type get_allocator() const noexcept {
        return asio::get_associated_allocator(handler_);
    }
};

struct initiate_async_wait_notification_op {
    template <typename Handler, typename Connection>
    void operator()(Handler&& h, Connection&& conn) const {
        async_wait_notification_op{
            std::forward<Connection>(conn),
            std::forward<Handler>(h)
        }.perform();
    }

    template <typename Handler, typename Connection, typename TimeConstraint>
    void operator()(Handler&& h, Connection&& conn, TimeConstraint t) const {
        if constexpr (t != none) {
            using real_connection = std::decay_t<decltype(unwrap_connection(conn))>;
            using deadline_handler = detail::io_deadline_handler<real_connection, std::decay_t<Handler>, Connection>;

            auto handler = deadline_handler{unwrap_connection(conn), t, std::forward<Handler>(h)};
            (*this)(std::move(handler), std::forward<Connection>(conn));
        } else {
            (*this)(std::forward<Handler>(h), std::forward<Connection>(conn));
        }
    }
};

} // namespace detail

template <typename Connection, typename TimeConstraint, typename CompletionToken>
auto unlisten_op::operator() (Connection&& conn, TimeConstraint t, CompletionToken&& token) const {
    static_assert(ozo::Connection<Connection>, "conn should model Connection");
    static_assert(ozo::TimeConstraint<TimeConstraint>, "should model TimeConstraint concept");

    return ozo::execute(std::forward<Connection>(conn), "UNLISTEN"_SQL, t,
        std::forward<CompletionToken>(token));
}

template <typename Connection, typename TimeConstraint, typename CompletionToken>
auto wait_notification_op::operator() (Connection&& conn, TimeConstraint t,
        CompletionToken&& token) const {
    static_assert(ozo::Connection<Connection>, "conn should model Connection");
    static_assert(ozo::TimeConstraint<TimeConstraint>, "should model TimeConstraint concept");

    return async_initiate<CompletionToken, handler_signature<std::decay_t<Connection>>>(
        detail::initiate_async_wait_notification_op, std::forward<CompletionToken>(token),
        std::forward<Connection>(conn), t);
}

} // namespace ozo
