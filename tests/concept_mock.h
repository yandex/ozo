#pragma once

#include <ozo/type_traits.h>

namespace ozo {
namespace testing {

class concept {
public:
    virtual ~concept() {}
};

template <typename T>
class nullable : public T {
};

template <typename T>
class emplaceable : public T {
public:
    virtual void emplace() = 0;
};

template <typename T>
class operator_not : public T {
public:
    virtual bool negate() const = 0;
    bool operator!() const { return negate(); }
};

template <typename E, typename T>
class has_element : public T {
public:
    using element_type = E;
};

template <typename T>
class move_assignable : public T {
    using self_t = move_assignable<T>;
public:
    virtual void move_assign() = 0;
    self_t& operator=(self_t&&) { move_assign(); return *this; }
};

template <typename T>
class resettable : public T {
public:
    virtual void reset() = 0;
};

} // namespace testing

template <typename T>
struct is_nullable<T, typename std::enable_if_t<
    std::is_base_of<testing::nullable<testing::concept>, T>::value
>> : ::std::true_type {};

} // namespace ozo
