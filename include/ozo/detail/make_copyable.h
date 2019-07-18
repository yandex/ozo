#pragma once

#include <ozo/asio.h>

#include <utility>
#include <type_traits>

namespace ozo::detail {

/**
 * @brief Wrapper to make Handler object copyable
 *
 * Sometimes asynchronous API does make a copy of completion handlers
 * (e.g. at that moment resource_pool does). In this case to do not obligate
 * user to use only copyable completion handlers it is better to provide
 * convenient wrapper for make a completion handler copyable.
 *
 * @tparam Handler
 */
template <typename Handler>
struct make_copyable {
    using target_type = std::decay_t<Handler>;
    using type = std::conditional_t<std::is_copy_constructible_v<target_type>,
                    target_type,
                    make_copyable>;

    using executor_type = typename asio::associated_executor<target_type>::type;

    executor_type get_executor() const noexcept {
        return asio::get_associated_executor(*handler_);
    }

    using allocator_type = typename asio::associated_allocator<target_type>::type;

    allocator_type get_allocator() const noexcept {
        return asio::get_associated_allocator(*handler_);
    }

    make_copyable(Handler&& handler)
    : make_copyable(std::allocator_arg, asio::get_associated_allocator(handler), std::move(handler)) {}

    template <typename Allocator, typename ...Ts>
    make_copyable(const std::allocator_arg_t&, const Allocator& alloc, Ts&& ...vs)
    : handler_(std::allocate_shared<target_type>(alloc, std::forward<Ts>(vs)...)) {}

    template <typename ...Args>
    auto operator()(Args&& ...args) & {
        return static_cast<target_type&>(*this)(std::forward<Args>(args)...);
    }

    template <typename ...Args>
    auto operator()(Args&& ...args) const & {
        return static_cast<const target_type&>(*this)(std::forward<Args>(args)...);
    }

    template <typename ...Args>
    auto operator()(Args&& ...args) && {
        return static_cast<target_type&&>(static_cast<make_copyable&&>(*this))(std::forward<Args>(args)...);
    }

    operator const target_type& () const & { return *handler_;}

    operator target_type& () & { return *handler_;}

    operator target_type&& () && { return static_cast<target_type&&>(*handler_);}

    std::shared_ptr<target_type> handler_;
};

template <typename Handler>
using make_copyable_t = typename make_copyable<Handler>::type;

} // namespace ozo::detail
