#pragma once

#include <ozo/type_traits.h>
#include <ozo/asio.h>
#include <ozo/deadline.h>
#include <ozo/connection.h>

#include <tuple>


/**
 * @defgroup group-failover Failover
 * @brief Failover microframwork for database-related operations
 */

/**
 * @defgroup group-failover-strategy Strategy interface
 * @ingroup group-failover
 * @brief Failover microframwork strategy extention interface
 */


#ifdef OZO_DOCUMENTATION
namespace ozo {
/**
 * @brief FailoverStrategy concept
 *
 * Failover strategy defines interface for the failover framework extention. `FailoverStrategy`
 * should be a factory for the first #FailoverTry.
 *
 * ### Failover Compatible Operation
 *
@code

#include <boost/asio/spawn.hpp>

struct complex_transaction_data {
//...
};

template <typename Initiator>
struct complex_transaction_op : base_async_operation <complex_transaction_op, Initiator> {
    using base = typename complex_transaction_op::base;
    using base::base;

    template <typename P, typename TimeConstraint, typename CompletionToken>
    decltype(auto) operator() (P&& provider, complex_transaction_data data, TimeConstraint t, CompletionToken&& token) const {
        static_assert(ozo::ConnectionProvider<P>, "provider should be a ConnectionProvider");
        static_assert(ozo::TimeConstraint<TimeConstraint>, "should model TimeConstraint concept");
        return ozo::async_initiate<CompletionToken, ozo::handler_signature<P>>(
            // Initiator and token
            get_operation_initiator(*this), token,
            // ConnectionProvider, TimeConstraint, additional arguments
            std::forward<P>(provider), ozo::deadline(t), std::move(data)
        );
    }
};

namespace detail {
template <typename Handler, typename TimeConstraint>
struct complex_transaction_impl {
    complex_transaction_data data_;
    TimeConstraint time_constraint_;
    Handler handler_;

    template <typename Connection>
    constexpr void operator()(error_code ec, Connection&& conn) {
        if (ec) {
            return handler_(ec, std::forward<Connection>(conn));
        }
        boost::asio::spawn(ozo::get_executor(conn), [
                data = std::move(data_),
                time_constraint = std::move(time_constraint_),
                handler = std::move(handler_)] (auto yield) mutable {
            error_code ec;
            ozo::rows_of<...> rows;
            auto conn = ozo::request(conn, time_constraint_, "..."_SQL..., ozo::into(rows), yield[ec]);
            if (!ec) {
                conn = ozo::execute(conn, time_constraint_, "..."_SQL + handle_data(rows)..., yield[ec]);
                if (!ec) {
                    return ozo::commit(conn, std::move(handler_));
                }
            }
            auto ex = boost::asio::get_associated_executor(handler_);
            boost::asio::dispatch(ex, [ec, handler=std::move(handler_), conn = std::move(conn)]{
                handler(ec, std::move(conn));
            });
        });
    }

    using executor_type = decltype(asio::get_associated_executor(handler_));

    executor_type get_executor() const noexcept {
        return asio::get_associated_executor(handler_);
    }

    using allocator_type = decltype(asio::get_associated_allocator(handler_));

    allocator_type get_allocator() const noexcept {
        return asio::get_associated_allocator(handler_);
    }
};

struct initiate_complex_transaction {
    template <typename Handler, typename P, typename Q, typename TimeConstraint>
    constexpr void operator()(Handler&& h, P&& provider, TimeConstraint t, complex_transaction_data data) const {
        ozo::begin(provider, t, complex_transaction_impl{std::move(data), t, std::forward<Handler>(h)});
    }
};
} // namespace detail

constexpr complex_transaction_op<detail::initiate_async_execute> complex_transaction;
@endcode
 * @ingroup group-failover-strategy
 * @hideinitializer
 */
template <typename T>
constexpr auto FailoverStrategy = std::false_type;

/**
 * @brief FailoverTry concept
 *
 * Failover try defines interface for a single operation execution try and possible
 * next failover try.
 *
 * @ingroup group-failover-strategy
 * @hideinitializer
 */
template <typename T>
constexpr auto FailoverTry = std::false_type;
}
#endif

namespace ozo::failover {

namespace hana = boost::hana;

/**
 * @brief Basic operation context
 *
 * @tparam ConnectionProvider --- #ConnectionProvider type.
 * @tparam TimeConstraint --- operation #TimeConstraint type.
 * @tparam Args --- other operation arguments' type.
 * @ingroup group-failover-strategy
 */
template <
    typename ConnectionProvider,
    typename TimeConstraint,
    typename ...Args>
struct basic_context {
    static_assert(ozo::TimeConstraint<TimeConstraint>, "TimeConstraint should models ozo::TimeConstraint");
    static_assert(ozo::ConnectionProvider<ConnectionProvider>, "ConnectionProvider should models ozo::ConnectionProvider");

    std::decay_t<ConnectionProvider> provider; //!< #ConnectionProvider for an operation, typically deduced from operation's 1st argument.
    TimeConstraint time_constraint; //!< #TimeConstraint for an operation, typically deduced from operation's 2nd argument.
    hana::tuple<std::decay_t<Args>...> args; //!< Other arguments of an operation except #CompletionToken.

    /**
     * @brief Construct a new basic context object
     *
     * @param p --- #ConnectionProvider for an operation.
     * @param t --- #TimeConstraint for an operation.
     * @param args --- other arguments of an operation except #CompletionToken.
     */
    basic_context(ConnectionProvider p, TimeConstraint t, Args ...args)
    : provider(std::forward<ConnectionProvider>(p)),
      time_constraint(t),
      args(std::forward<Args>(args)...) {}
};

template <typename FailoverStrategy, typename Operation>
struct get_first_try_impl {
    template <typename Allocator, typename ...Args>
    static auto apply(const Operation& op, const FailoverStrategy& s, const Allocator& alloc, Args&& ...args) {
        return s.get_first_try(op, alloc, std::forward<Args>(args)...);
    }
};

/**
 * @brief Get the first try object for an operation
 *
 * This function is a part of failover strategy interface. It creates the first try of operation
 * execution context. The context data should be allocated via the specified allocator. This function
 * would be called once during a failover operation execution. By default #FailoverStrategy should
 * have `get_first_try(const Operation& op, const Allocator& alloc, Args&& ...args) const` member function.
 *
 * @param op --- operation for which the try object should be created.
 * @param strategy --- strategy according to which the try object should be created.
 * @param alloc --- allocator for try object.
 * @param args --- operation arguments except #CompletionToken.
 * @return #FailoverTry object.
 *
 * ###Customization Point
 *
 * This function may be customized for a #FailoverStrategy via specialization
 * of `ozo::failover::get_first_try_impl`. E.g.:
 * @code
namespace ozo::failover {

template <typename Operation>
struct get_first_try_impl<my_strategy, Operation> {
    template <typename Allocator, typename ...Args>
    static auto apply(const Operation&, const my_strategy& s, const Allocator& alloc, Args&& ...args) {
        return s.get_start_context<Operation>(alloc, std::forward<Args>(args)...);
    }
};

} // namespace ozo::failover
 * @endcode
 * @ingroup group-failover-strategy
 */
template <typename FailoverStrategy, typename Operation, typename Allocator, typename ...Args>
inline auto get_first_try(const Operation& op, const FailoverStrategy& strategy, const Allocator& alloc, Args&& ...args) {
    return get_first_try_impl<FailoverStrategy, Operation>::apply(op, strategy, alloc, std::forward<Args>(args)...);
}

template <typename Try>
struct get_try_context_impl {
    static decltype(auto) apply(const Try& a_try) {
        return a_try.get_context();
    }
};

/**
 * @brief Get operation context for the try
 *
 * By default #FailoverTry object should have `get_context() const` member function.
 *
 * @param a_try --- #FailoverTry object
 * @return auto --- #HanaSequence with operation arguments.
 *
 * ###Customization Point
 *
 * This function may be customized for a #FailoverTry via specialization
 * of `ozo::failover::get_try_context_impl`. E.g.:
 * @code
namespace ozo::failover {

template <>
struct get_try_context_impl<my_strategy_try> {
    template <typename Allocator, typename ...Args>
    static auto apply(const my_strategy_try& obj) {
        return obj.ctx;
    }
};

} // namespace ozo::failover
 * @endcode
 * @ingroup group-failover-strategy
 */
template <typename FailoverTry>
inline auto get_try_context(const FailoverTry& a_try) {
    auto res = detail::apply<get_try_context_impl>(ozo::unwrap(a_try));
    static_assert(HanaSequence<decltype(res)>,
        "get_try_context_impl::apply() should provide HanaSequence");
    return res;
}

template <typename Try>
struct get_next_try_impl {
    template <typename Conn>
    static auto apply(Try& a_try, error_code ec, Conn&& conn) {
        return a_try.get_next_try(ec, conn);
    }
};

/**
 * @brief Get the next try object
 *
 * Return #FailoverTry for next failover try if it possible. By default it calls
 * `a_try.get_next_try(ec, conn)`. This behavior customisable via `ozo::failover::get_next_try_impl`
 * specialization. It will be used by default if no `ozo::failover::initiate_next_try`
 * customization had been made.
 *
 * @param a_try --- current #FailoverTry object.
 * @param ec --- current try error code.
 * @param conn --- current #connection object.
 * @return #FailoverTry --- if given error code and connection may be failovered with next try.
 * @return null-state --- otherwise.
 *
 * ###Customization Point
 *
 * This function may be customized for a #FailoverTry via specialization
 * of `ozo::failover::get_next_try_impl`. E.g.:
 * @code
namespace ozo::failover {

template <>
struct get_next_try_impl<my_strategy_try> {
    template <typename Allocator, typename ...Args>
    static std::optional<my_strategy_try> apply(const my_strategy_try& obj, error_code ec, Conn&& conn) {
        if (ec != my_error_condition::recoverable) {
            obj.log_error("can not recover error {0} with message {1}", ec, ozo::error_message(conn));
            return std::nullopt;
        }
        obj.log_notice("recovering error {0} with message {1}", ec, ozo::error_message(conn));
        return obj;
    }
};

} // namespace ozo::failover
 * @endcode
 * @ingroup group-failover-strategy
 */
template <typename Try, typename Connection>
inline auto get_next_try(Try& a_try, const error_code& ec, Connection& conn) {
    return detail::apply<get_next_try_impl>(ozo::unwrap(a_try), ec, conn);
}

template <typename Try>
struct initiate_next_try_impl {
    template <typename Connection, typename Initiator>
    static auto apply(Try& a_try, const error_code& ec, Connection& conn, Initiator init) {
        if (auto next = get_next_try(a_try, ec, conn); !ozo::is_null(next)) {
            init(std::move(next));
        }
    }
};

/**
 * @brief Initiates the next try of an operation execution
 *
 * This mechanism supports static polymorphism of tries depended on error conditions.
 * Unlike `ozo::failover::get_next_try()` it allows to use different types of
 * try-objects for different error conditions.
 *
 * Function implementation should call initiator in case of operation with given error
 * condition may be recovered by try-object is given to initiator. Or should not call
 * initiator in case of an unrecoverable error condition.
 *
 * @note Initiator object should be called only once and should not be called out of the
 * function scope since it contains references on stack-allocated objects.
 *
 * @param a_try --- current #FailoverTry object.
 * @param ec --- current try error code.
 * @param conn --- current #connection object.
 * @param init --- initiator of an operation execution, functional object with signature `void(auto try)`
 *
 * ### Customization Point
 *
 * This function may be customized for a #FailoverTry via specialization
 * of `ozo::failover::initiate_next_try_impl`. E.g. default implementation may look like this:
 * @code
namespace ozo::failover {

template <typename Try>
struct initiate_next_try_impl {
    template <typename Connection, typename Initiator>
    static auto apply(Try& a_try, const error_code& ec, Connection& conn, Initiator init) {
        if (auto next = get_next_try(a_try, ec, conn); !is_null(next)) {
            init(std::move(next));
        }
    }
};

} // namespace ozo::failover
 * @endcode
 * @ingroup group-failover-strategy
 */
template <typename Try, typename Connection, typename Initiator>
inline auto initiate_next_try(Try& a_try, const error_code& ec, Connection& conn, Initiator&& init) {
    return detail::apply<initiate_next_try_impl>(ozo::unwrap(a_try), ec, conn, std::forward<Initiator>(init));
}

namespace detail {

template <template<typename...> typename Template, typename Allocator, typename ...Ts>
static auto allocate_shared(const Allocator& alloc, Ts&& ...vs) {
    using type = decltype(Template{std::forward<Ts>(vs)...});
    return std::allocate_shared<type>(alloc, std::forward<Ts>(vs)...);
}

template <typename Try, typename Operation, typename Handler>
inline void initiate_operation(const Operation&, Try&&, Handler&&);

template <typename Operation, typename Try, typename Handler>
struct continuation {
    Operation op_;
    Try try_;
    Handler handler_;

    continuation(const Operation& op, Try a_try, Handler handler)
    : op_(op), try_(std::move(a_try)), handler_(std::move(handler)) {}

    template <typename Connection>
    void operator() (error_code ec, Connection&& conn) {
        static_assert(ozo::Connection<Connection>, "conn should model Connection concept");
        if (ec) {
            bool initiated = false;

            initiate_next_try(try_, ec, conn, [&] (auto next_try) {
                initiate_operation(op_, std::move(next_try), std::move(handler_));
                initiated = true;
            });

            if (initiated) {
                return;
            }
        }
        handler_(std::move(ec), std::forward<Connection>(conn));
    }

    using executor_type = decltype(asio::get_associated_executor(handler_));

    executor_type get_executor() const {
        return asio::get_associated_executor(handler_);
    }

    using allocator_type = decltype(asio::get_associated_allocator(handler_));

    allocator_type get_allocator() const {
        return asio::get_associated_allocator(handler_);
    }
};

template <typename Try, typename Operation, typename Handler>
inline void initiate_operation(const Operation& op, Try&& a_try, Handler&& handler) {
    hana::unpack(get_try_context(a_try), [&](auto&& ...args) {
        ozo::get_operation_initiator(op)(
            continuation{op, std::forward<Try>(a_try), std::forward<Handler>(handler)},
            std::forward<decltype(args)>(args)...
        );
    });
}

template <typename FailoverStrategy, typename Operation>
struct operation_initiator {
    FailoverStrategy strategy_;
    Operation op_;

    constexpr operation_initiator(FailoverStrategy strategy, const Operation& op)
    : strategy_(std::move(strategy)), op_(op) {}

    template <typename Handler, typename ...Args>
    void operator() (Handler&& handler, Args&& ...args) const {
        auto first_try = get_first_try(op_, strategy_, asio::get_associated_allocator(handler), std::forward<Args>(args)...);
        initiate_operation(op_, std::move(first_try), std::forward<Handler>(handler));
    }
};

} // namespace detail

struct construct_initiator_impl {
    template <typename FailoverStrategy, typename Op>
    constexpr static auto apply(FailoverStrategy&& strategy, const Op& op) {
        return detail::operation_initiator(std::forward<FailoverStrategy>(strategy), op);
    }
};
} // namespace ozo::failover
