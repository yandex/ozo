#pragma once

#include <ozo/core/concept.h>
#include <ozo/detail/functional.h>
#include <type_traits>
#include <memory>

namespace ozo {

template <typename T, typename Enable = void>
struct is_nullable : std::false_type {};

/**
 * @brief Indicates if type meets nullable requirements.
 *
 * `Nullable` type has to:
 * * have a null-state,
 * * be `bool` convertable - `false` indicates null state,
 * * be dereferenceable via `operator *`,
 * * have `allocate_nullable()` specialization (see the function documentation for how to).
 *
 * These next types are `Nullable` out of the box:
 * * `boost::optional`,
 * * `std::optional`,
 * * `boost::scoped_ptr`,
 * * `std::unique_ptr`,
 * * `boost::shared_ptr`,
 * * `std::shared_ptr`,
 * * `boost::weak_ptr`,
 * * `std::weak_ptr`.
 *
 * @note Raw pointers are not supported as `Nullable` by default, because this library uses RAII model for objects and
 * no special deallocation functions are used. But if it is really needed and you want to deallocate the allocated
 * objects manually you can add them as described below.
 *
 * ### Example
 *
 * If you want to add nullable type to the library --- you have to specialize `ozo::is_nullable` struct and specify allocation function like this:
 *
 * @code
//Add raw pointer to nullables

namespace ozo {

// Mark raw pointer as nullable
template <typename T>
struct is_nullable<T*> : std::true_type {};

// Specify allocation function
template <typename T>
struct allocate_nullable_impl<T*> {
    template <typename Alloc>
    static void apply(T*& out, const Alloc&) {
        out = new T();
    }
};

}

 * @endcode
 *
 * @sa ozo::unwrap(), is_null(), allocate_nullable(), init_nullable(), reset_nullable()
 * @ingroup group-core-concepts
 * @hideinitializer
 */
template <typename T>
constexpr auto Nullable = is_nullable<std::decay_t<T>>::value;

template <typename T>
struct is_null_impl : std::conditional_t<Nullable<T>,
    detail::functional::operator_not,
    detail::functional::always_false
> {};

/**
 * @brief Indicates if value is in null state
 *
 * This utility function indicates if given value is in null state.
 * If argument is not #Nullable it always returns `false`
 *
 * @param value --- object to examine
 * @return `true` --- object is in null-state,
 * @return `false` --- overwise.
 * @ingroup group-core-functions
 */
template <typename T>
constexpr decltype(auto) is_null(const T& v) noexcept(noexcept(is_null_impl<T>::apply(v))) {
    return is_null_impl<T>::apply(v);
}

template <typename T, typename = std::void_t<>>
struct allocate_nullable_impl {
    static_assert(Emplaceable<T>, "default implementation uses emplace() method");
    template <typename Alloc>
    static void apply(T& out, const Alloc&) {
        out.emplace();
    }
};

template <typename T, typename Alloc>
inline void allocate_nullable(T& out, const Alloc& a) {
    static_assert(Nullable<T>, "T must be nullable");
    allocate_nullable_impl<std::decay_t<T>>::apply(out, a);
}

template <typename T, typename Alloc = std::allocator<char>>
inline void init_nullable(T& n, const Alloc& a = Alloc{}) {
    static_assert(Nullable<T>, "T must be nullable");
    if (is_null(n)) {
        allocate_nullable(n, a);
    }
}

template <typename T>
inline void reset_nullable(T& n) {
    static_assert(Nullable<T>, "T must be nullable");
    n = T{};
}

} // namespace ozo
