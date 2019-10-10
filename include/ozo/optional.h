#pragma once

#if defined(__has_include)
#   if __has_include(<optional>)
#       define OZO_HAS_STD_OPTIONAL 1
#   elif __has_include(<experimental/optional>)
#       define OZO_HAS_STD_EXPIREMENTAL_OPTIONAL 1
#   endif
#elif
#   define OZO_HAS_STD_OPTIONAL 1
#endif


#if defined(OZO_HAS_STD_OPTIONAL)
#   define OZO_STD_OPTIONAL std::optional
#   define OZO_NULLOPT_T std::nullopt_t
#   define OZO_NULLOPT std::nullopt
#   include <optional>
#elif defined(OZO_HAS_STD_EXPIREMENTAL_OPTIONAL)
#   define OZO_STD_OPTIONAL std::experimental::optional
#   define OZO_NULLOPT_T std::experimental::nullopt_t
#   define OZO_NULLOPT std::experimental::nullopt
#   include <experimental/optional>
#endif
