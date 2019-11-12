# find_program(CCACHE_FOUND ccache)

# if(CCACHE_FOUND)
#     set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
#     set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
# endif()

add_executable(ozo_benchmark ozo_benchmark.cpp)
target_link_libraries(ozo_benchmark ozo)

# enable a bunch of warnings and make them errors
target_compile_options(ozo_benchmark PRIVATE -Wall -Wextra -Wsign-compare -pedantic -Werror)

# ignore specific error for clang
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    target_compile_options(ozo_benchmark PRIVATE -Wno-ignored-optimization-argument)
endif()

add_executable(ozo_benchmark_performance performance.cpp)
target_link_libraries(ozo_benchmark_performance ozo)

# enable a bunch of warnings and make them errors
target_compile_options(ozo_benchmark_performance PRIVATE -Wall -Wextra -Wsign-compare -pedantic -Werror)

# ignore specific error for clang
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    target_compile_options(ozo_benchmark_performance PRIVATE -Wno-ignored-optimization-argument)
endif()

if(OZO_COMPILE_TIME_BENCHMARK)
    # Actually include the module
    include(metabench)

    # Add new datasets
    metabench_add_dataset(request_dataset "request.cpp.erb" "[1, 2, 5, 10, 20, 50, 100, 200, 500]" MEDIAN_OF 3)
    metabench_add_dataset(retry_dataset "retry.cpp.erb" "[1, 2, 5, 10, 20, 50, 100, 200, 500]" MEDIAN_OF 3)

    # Add a new chart
    metabench_add_chart(chart DATASETS request_dataset retry_dataset)
    target_link_libraries(request_dataset ozo)
    target_link_libraries(retry_dataset ozo)
endif()
