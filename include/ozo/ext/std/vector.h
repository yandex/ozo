#pragma once

#include <ozo/type_traits.h>
#include <vector>

namespace ozo {

template <typename ...Ts>
struct is_array<std::vector<Ts...>> : std::true_type {};

}
