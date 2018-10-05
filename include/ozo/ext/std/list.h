#pragma once

#include <ozo/type_traits.h>
#include <list>

namespace ozo {

template <typename ...Ts>
struct is_array<std::list<Ts...>> : std::true_type {};

}
