find_program(CCACHE_FOUND ccache)

if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif()

add_executable(ozo_benchmark ozo_benchmark.cpp)
target_link_libraries(ozo_benchmark ozo)

find_package(Boost COMPONENTS coroutine context system thread atomic program_options REQUIRED)

# enable a bunch of warnings and make them errors
target_compile_options(ozo_benchmark PRIVATE -Wall -Wextra -Wsign-compare -pedantic -Werror)

# ignore specific error for clang
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    target_compile_options(ozo_benchmark PRIVATE -Wno-ignored-optimization-argument)
endif()

add_executable(ozo_benchmark_performance performance.cpp)
target_link_libraries(ozo_benchmark_performance ozo)
target_link_libraries(ozo_benchmark_performance Boost::program_options)

# enable a bunch of warnings and make them errors
target_compile_options(ozo_benchmark_performance PRIVATE -Wall -Wextra -Wsign-compare -pedantic -Werror)

# ignore specific error for clang
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    target_compile_options(ozo_benchmark_performance PRIVATE -Wno-ignored-optimization-argument)
endif()
