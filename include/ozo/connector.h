#pragma once

#include <ozo/asio.h>
#include <ozo/connection.h>

namespace ozo {

template <typename Provider>
class bind_get_connection_timeout {
public:
    static_assert(ConnectionProvider<Provider>, "Provider should model ConnectionProvider concept");

    using duration = time_traits::duration;
    using target_type = Provider;
    using connection_type = ozo::connection_type<target_type>;

    bind_get_connection_timeout(Provider target, duration timeout)
    : target_(std::move(target)), timeout_(timeout) {}

    template <typename TimeConstraint, typename Handler>
    void async_get_connection(TimeConstraint t, Handler&& handler) const& {
        static_assert(ozo::TimeConstraint<TimeConstraint>, "should model TimeConstraint concept");
        ozo::async_get_connection(target(), timeout(t), std::forward<Handler>(handler));
    }

    template <typename TimeConstraint, typename Handler>
    void async_get_connection(TimeConstraint t, Handler&& handler) & {
        static_assert(ozo::TimeConstraint<TimeConstraint>, "should model TimeConstraint concept");
        ozo::async_get_connection(target(), timeout(t), std::forward<Handler>(handler));
    }

    template <typename TimeConstraint, typename Handler>
    void async_get_connection(TimeConstraint t, Handler&& handler) && {
        static_assert(ozo::TimeConstraint<TimeConstraint>, "should model TimeConstraint concept");
        ozo::async_get_connection(std::move(target()), timeout(t), std::forward<Handler>(handler));
    }

    target_type& target() & { return target_; }
    target_type& target() && { return target_; }
    const target_type& target() const & { return target_; }
    constexpr duration timeout() const {return timeout_;}
    constexpr duration timeout(none_t) const {return timeout_;}
    constexpr duration timeout(time_traits::time_point t) const {return std::min(timeout(), time_left(t));}
    constexpr duration timeout(time_traits::duration t) const {return std::min(timeout(), t);}
private:
    target_type target_;
    duration timeout_;
};

/**
 * @brief Default implementation of ConnectionProvider
 *
 * This is the default implementation of the #ConnectionProvider concept. It binds
 * `io_context` to a #ConnectionSource implementation object.
 *
 * Thus `connection_provider` can create connection via #ConnectionSource object running its
 * asynchronous connect operation on the `io_context` with additional parameters.
 * As a result, `connection_provider` provides a #Connection object bound to `io_context` via
 * `ozo::get_connection()` function.
 *
 * @tparam Source --- #ConnectionSource implementation
 * @ingroup group-connection-types
 */
template <typename Source>
class connection_provider {
public:
    static_assert(ConnectionSource<Source>, "is not a ConnectionSource");

    /**
     * Source type according to #ConnectionProvider requirements
     */
    using source_type = Source;
    /**
     * #Connection implementation type according to #ConnectionProvider requirements.
     * Specifies the #Connection implementation type which can be obtained from this provider.
     */
    using connection_type = typename connection_source_traits<source_type>::connection_type;

    /**
     * Construct a new `connection_provider` object
     *
     * @param source --- #ConnectionSource implementation
     * @param io --- `io_context` for asynchronous IO
     */
    template <typename T>
    connection_provider(T&& source, io_context& io)
     : source_(std::forward<T>(source)), io_(io) {
    }

    template <typename TimeConstraint, typename Handler>
    void async_get_connection(TimeConstraint t, Handler&& h) const& {
        static_assert(ozo::TimeConstraint<TimeConstraint>, "should model TimeConstraint concept");
        source_(io_, std::move(t), std::forward<Handler>(h));
    }

    template <typename TimeConstraint, typename Handler>
    void async_get_connection(TimeConstraint t, Handler&& h) & {
        static_assert(ozo::TimeConstraint<TimeConstraint>, "should model TimeConstraint concept");
        source_(io_, std::move(t), std::forward<Handler>(h));
    }

    template <typename TimeConstraint, typename Handler>
    void async_get_connection(TimeConstraint t, Handler&& h) && {
        static_assert(ozo::TimeConstraint<TimeConstraint>, "should model TimeConstraint concept");
        std::forward<Source>(source_)(io_, std::move(t), std::forward<Handler>(h));
    }

private:
    source_type source_;
    io_context& io_;
};

template <typename T>
connection_provider(T&& source, io_context& io) -> connection_provider<T>;

} // namespace ozo

