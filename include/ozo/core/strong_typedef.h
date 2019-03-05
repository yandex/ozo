#pragma once

#include <boost/operators.hpp>
#include <type_traits>

namespace ozo {

template <typename T, typename Tag>
class strong_typedef_wrapper
    : boost::totally_ordered1<strong_typedef_wrapper<T, Tag>
    , boost::totally_ordered2<strong_typedef_wrapper<T, Tag>, T>> {

    T base_;
public:
    using base_type = T;

    explicit strong_typedef_wrapper(const base_type& v) noexcept(noexcept(base_type(v)))
        : base_(v) {}
    explicit strong_typedef_wrapper(base_type&& v) noexcept(noexcept(base_type(v)))
        : base_(v) {}

    strong_typedef_wrapper() = default;
    strong_typedef_wrapper(const strong_typedef_wrapper&) = default;
    strong_typedef_wrapper(strong_typedef_wrapper&&) = default;

    strong_typedef_wrapper& operator = (const strong_typedef_wrapper&) = default;
    strong_typedef_wrapper& operator = (strong_typedef_wrapper&&) = default;

    strong_typedef_wrapper& operator = (const base_type& rhs) noexcept(noexcept(base_ = rhs)) {
        base_ = rhs;
        return *this;
    }

    strong_typedef_wrapper& operator = (base_type&& rhs) noexcept(noexcept(base_ = std::move(rhs))) {
        base_ = std::move(rhs);
        return *this;
    }

    constexpr const base_type& get() const & noexcept { return base_;}
    constexpr base_type& get() & noexcept { return base_;}
    constexpr base_type&& get() && noexcept { return std::move(base_);}

    operator const base_type&() const & noexcept {return get();}
    operator base_type&() & noexcept {return get();}
    operator base_type&&() && noexcept {return get();}

    bool operator == (const strong_typedef_wrapper& rhs) const {return base_ == rhs.base_;}
    bool operator < (const strong_typedef_wrapper& rhs) const {return base_ < rhs.base_;}
};

template <typename T>
struct is_strong_typedef : std::false_type {};

template <typename T, typename Tag>
struct is_strong_typedef<strong_typedef_wrapper<T, Tag>> : std::true_type {};

template <typename T>
constexpr auto StrongTypedef = is_strong_typedef<std::decay_t<T>>::value;

} // namespace ozo

/**
 * @brief Strong typedef
 * @ingroup group-core-types
 *
 * C++ `typedef` creates only an alias to a base type, so the both types are the same.
 * To get a really new type it is necessary to make some boilerplate code.
 * This macro do such work. It is very similar to `BOOST_STRONG_TYPEDEF`
 * Except the new type has `base_type` type and `get()` method which provides
 * access to underlying base type object.
 * @note This macro may be used inside a namespace.
 * @param base --- base type
 * @param type --- new type
 *
 * ### Example
 *
 * @code

namespace demo {

OZO_STRONG_TYPEDEF(std::vector<char>, bytes)

}
// Types are completely different
static_assert(!std::is_same_v<std::vector<char>, demo::bytes>);
// Type bytes::base_type is same as a first argument of the macro
static_assert(std::is_same_v<std::vector<char>, demo::bytes::base_type>);

demo::bytes b{};
std::vector<char>& base(b);

// Member function bytes::get() provides access to the underlying
// base type object
assert(std::addressof(base) == std::addressof(b.get()));
 * @endcode
 */
#ifdef OZO_DOCUMENTATION
#define OZO_STRONG_TYPEDEF(base, type)
#else
#define OZO_STRONG_TYPEDEF(base, type)\
    struct type##_strong_typedef_tag;\
    using type = ozo::strong_typedef_wrapper<base, type##_strong_typedef_tag>;
#endif
