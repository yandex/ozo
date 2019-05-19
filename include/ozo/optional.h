#pragma once

#if defined(__has_include)
#   if __has_include(<optional>)
#       define __OZO_HAS_STD_OPTIONAL 1
#   elif __has_include(<experimental/optional>)
#       define __OZO_HAS_STD_EXPIREMENTAL_OPTIONAL 1
#   endif
#elif
#   define __OZO_HAS_STD_OPTIONAL 1
#endif


#if defined(__OZO_HAS_STD_OPTIONAL)
#   define __OZO_STD_OPTIONAL std::optional
#   define __OZO_NULLOPT_T std::nullopt_t
#   define __OZO_NULLOPT std::nullopt
#   include <optional>
#elif defined(__OZO_HAS_STD_EXPIREMENTAL_OPTIONAL)
#   define __OZO_STD_OPTIONAL std::experimental::optional
#   define __OZO_NULLOPT_T std::experimental::nullopt_t
#   define __OZO_NULLOPT std::experimental::nullopt
#   include <experimental/optional>
#endif

