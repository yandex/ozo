#pragma once

#include <ozo/core/concept.h>
#include <ozo/detail/functional.h>
#include <type_traits>
#include <memory>

namespace ozo {

template <typename T, typename Enable = void>
struct is_nullable : std::false_type {};

/**
 * @brief Indicates if type is marked as conformed to nullable requirements.
 *
 * These next types are defined as `Nullable` via @ref group-ext of the library:
 * * @ref group-ext-boost-optional,
 * * @ref group-ext-boost-scoped_ptr,
 * * @ref group-ext-boost-shared_ptr,
 * * @ref group-ext-boost-weak_ptr,
 * * @ref group-ext-std-nullopt,
 * * @ref group-ext-std-nullptr,
 * * @ref group-ext-std-optional,
 * * @ref group-ext-std-shared_ptr,
 * * @ref group-ext-std-unique_ptr,
 * * @ref group-ext-std-weak_ptr.
 *
 * If it is needed to mark type as nullable then `ozo::is_nullable` from the type should
 * be specialized as `std::true_type` type. If the type has to be allocated in special way
 * --- `ozo::allocate_nullable()` should be specialized (see @ref group-ext-std-shared_ptr as an example).
 *
 * ### Example
 *
 * Raw pointers are not supported as `Nullable` by default, because this library uses RAII model for objects and
 * no special deallocation functions are used. But if it is really needed and you want to deallocate the allocated
 * objects manually you can add them as described below.
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
 * @sa is_null(), allocate_nullable(), init_nullable(), reset_nullable()
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

/**
 * @brief Allocates nullable object of given type
 *
 * This function allocates memory and constructs nullable object of given type
 * by means of given allocator if applicable. It uses `ozo::allocate_nullable_impl`
 * functional object as an implementation.
 * @note The function implementation may use no allocator and ignore the argument
 * (e.g.: if it is not applicable to the given object type).
 *
 * @param out --- object to allocate to
 * @param a --- allocator to use
 * @ingroup group-core-functions
 */
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
