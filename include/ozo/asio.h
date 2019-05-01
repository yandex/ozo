#pragma once

#include <boost/asio/io_service.hpp>
#include <boost/asio/executor.hpp>
#include <boost/asio/strand.hpp>

namespace ozo {

namespace asio = boost::asio;
using asio::async_completion;
using asio::io_context;

#if BOOST_VERSION < 107000

template <typename CompletionToken, typename Signature,
    typename Initiation, typename... Args>
inline decltype(auto) async_initiate(Initiation&& initiation,
    CompletionToken& token, Args&&... args) {
  async_completion<CompletionToken, Signature> completion(token);

  initiation(std::move(completion.completion_handler), std::forward<Args>(args)...);

  return completion.result.get();
}

#else

using asio::async_initiate;

#endif

namespace detail {

template <typename Executor>
struct strand_executor {
    using type = asio::strand<Executor>;

    static auto get(const Executor& ex = Executor{}) {
        return type{ex};
    }
};

template <typename Executor>
using strand = typename strand_executor<std::decay_t<Executor>>::type;

template <typename Executor>
auto make_strand_executor(const Executor& ex) {
    return strand_executor<Executor>::get(ex);
}

} // namespace detail

template <typename Operation>
struct get_operation_initiator_impl {
    constexpr static auto apply(const Operation& op) {
        return op.get_initiator();
    }
};

/**
 * @brief Get asynchronous operation initiator
 *
 * Initiator is a functional object which may be used with
 * [boost::asio::async_initiate](https://www.boost.org/doc/libs/1_70_0/doc/html/boost_asio/reference/async_initiate.html)
 * to start an asynchronous operation. Typically this is detail zone of an operation,
 * but in OZO this is a part of operations customization for extentions like failover.
 *
 * @param op --- asynchronous operation object
 * @return initiator functional object
 *
 * ###Customization Point
 *
 * The function is implemented via `ozo::get_operation_initiator_impl`. The default
 * implementation is:
@code
template <typename Operation>
struct get_operation_initiator_impl {
    constexpr static auto apply(const Operation& op) {
        return op.get_initiator();
    }
};
@endcode
 *
 * So this behaviour may be changed via specialization of the template.
 * @ingroup group-core-functions
 */
template <typename Operation>
constexpr auto get_operation_initiator(const Operation& op) {
    return get_operation_initiator_impl<Operation>::apply(op);
}

template <typename Factory, typename Operation>
struct construct_initiator_impl {
    constexpr static auto apply(const Factory&, const Operation&) {
        static_assert(std::is_void_v<Factory>, "Factory is not supported for the Operation");
    }
};

/**
 * @brief Create asynchronous operation initiator using factory
 *
 * The function constructs asynchronous operation initiator using factory object.
 * The default behaviour is static assertion. The behaviour should be customized
 * for a particular factory.
 *
 * @param f --- factory for asynchronous operation initiator object.
 * @param op --- asynchronous operation object.
 * @return initiator functional object.
 *
 * ###Customization Point
 *
 * Default implementation is static assertion
 * @code
template <typename Factory, typename Operation>
struct construct_initiator_impl {
    constexpr static auto apply(const Factory&, const Operation&) {
        static_assert(std::is_void_v<Factory>, "Factory is not supported for the Operation");
    }
};
 * @endcode
 *
 * @ingroup group-core-functions
 */
template <typename Operation, typename Factory>
constexpr auto construct_initiator(Factory&& f, const Operation& op) {
    return construct_initiator_impl<std::decay_t<Factory>, Operation>::apply(f, op);
}

/**
 * @brief Base class for async operations
 *
 * Base class for async operation which provides facilities for initiator rebinding.
 * Should be used by all the operations which support extentions like failover via
 * initiator rebinding.
 *
 * ### Example
 *
 * Execute operation may look like this (simplified, for exposition only):
 *
 * @code
template <typename Initiator>
struct execute_op : base_async_operation <execute_op, Initiator> {
    using base = typename execute_op::base;
    using base::base;

    template <typename P, typename Q, typename TimeConstraint, typename CompletionToken>
    decltype(auto) operator() (P&& provider, Q&& query, TimeConstraint t, CompletionToken&& token) const {
        return async_initiate<CompletionToken, handler_signature<P>>(
            get_operation_initiator(*this), token, std::forward<P>(provider), t, std::forward<Q>(query));
    }
};

constexpr execute_op<impl::initiate_async_execute> execute;
 * @endcode
 * @ingroup group-core-types
 */
template <template<typename...> typename Operation, typename Initiator>
struct base_async_operation {
    using initiator_type = Initiator;
    using base = base_async_operation;
    initiator_type initiator_;

    constexpr base_async_operation(const Initiator& initiator = Initiator{}) : initiator_(initiator) {}

    constexpr initiator_type get_initiator() const { return initiator_;}

    template <typename OtherInitiator>
    constexpr static auto rebind_initiator(const OtherInitiator& other) {
        return Operation<OtherInitiator>{other};
    }

    template <typename InitiatorFactory>
    constexpr auto operator[] (InitiatorFactory&& f) const {
        return rebind_initiator(construct_initiator(std::forward<InitiatorFactory>(f), *this));
    }
};

} // namespace ozo
