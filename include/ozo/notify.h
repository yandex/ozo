#pragma once

#include <ozo/pg/handle.h>
#include <ozo/pg/connection.h>

#include <string_view>

namespace ozo {

class notification {
public:
    /**
     * Notification channel name
     */
    std::string_view relname() const noexcept {return v_->relname;}
    /**
     * Process ID of notifying server process
     */
    int backend_pid() const noexcept {return v_->be_pid;}
    /**
     * Notification payload string
     */
    std::string_view extra() const noexcept { return v_->extra;}
    /**
     * Returns true if notification is present
     */
    operator bool () const noexcept { return bool(v_);}
    bool operator!() const noexcept { return !v_;}
private:
    ozo::pg::shared_notify v_; // to be easy copyable
};

namespace deatil {
struct initiate_async_listen_op;
} // namespace detail

template <typename Initiator>
struct listen_op : base_async_operation <listen_op<Initiator, Options>, Initiator> {
    using base = typename listen_op::base;

    constexpr explicit listen_op(Initiator initiator = {}) : base(initiator) {}

    template <typename ConnectionProvider, typename CompletionToken>
    auto operator() (ConnectionProvider&& provider, CompletionToken&& token) const {
        return (*this)(std::forward<ConnectionProvider>(provider), none,
            std::forward<CompletionToken>(token));
    }

    template <typename ConnectionProvider, typename TimeConstraint, typename CompletionToken>
    auto operator() (ConnectionProvider&& provider, TimeConstraint t, CompletionToken&& token) const;

    template <typename OtherInitiator>
    constexpr auto rebind_initiator(const OtherInitiator& other) const {
        return listen_op<OtherInitiator>{other};
    }
};

inline constexpr listen_op<detail::initiate_async_listen_op> listen;

struct unlisten_op {
    template <typename Connection, typename CompletionToken>
    auto operator() (Connection&& conn, CompletionToken&& token) const {
        return (*this)(std::forward<Connection>(conn), none,
            std::forward<CompletionToken>(token));
    }

    template <typename Connection, typename TimeConstraint, typename CompletionToken>
    auto operator() (Connection&& conn, TimeConstraint t, CompletionToken&& token) const;
};

inline constexpr unlisten_op unlisten;

template <typename Connection>
inline notification get_notification(Connection& conn);

struct wait_notification_op {
    template <typename Connection, typename CompletionToken>
    auto operator() (Connection&& conn, CompletionToken&& token) const {
        return (*this)(std::forward<Connection>(conn), none, std::forward<CompletionToken>(token));
    }

    template <typename Connection, typename TimeConstraint, typename CompletionToken>
    auto operator() (Connection&& conn, TimeConstraint t, CompletionToken&& token) const;
};

inline constexpr wait_notification_op wait_notification;

} // namespace ozo

#include <ozo/impl/notify.h>
