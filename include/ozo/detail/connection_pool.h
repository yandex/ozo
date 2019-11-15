#pragma once

#include <ozo/core/thread_safety.h>
#include <ozo/detail/stub_mutex.h>

#include <yamail/resource_pool/async/pool.hpp>

#include <mutex>

namespace ozo::detail {

template <typename ConnectionRepType, typename ThreadSafety>
struct get_connection_pool_impl;

template <typename ConnectionRepType>
struct get_connection_pool_impl<ConnectionRepType, thread_safety<true>> {
    using type = yamail::resource_pool::async::pool<ConnectionRepType, std::mutex>;
};

template <typename ConnectionRepType>
struct get_connection_pool_impl<ConnectionRepType, thread_safety<false>> {
    using type = yamail::resource_pool::async::pool<ConnectionRepType, stub_mutex>;
};

template <typename ConnectionRepType, typename ThreadSafety>
using get_connection_pool_impl_t = typename get_connection_pool_impl<ConnectionRepType, std::decay_t<ThreadSafety>>::type;

} // namespace ozo::detail
