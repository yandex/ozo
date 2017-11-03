cmake_minimum_required(VERSION 2.8)

project(apq)

option(APQ_BUILD_TESTS "Enable apq tests build" OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBOOST_COROUTINES_NO_DEPRECATION_WARNING")

include_directories(contrib/resource_pool/include)
include_directories(include)

add_subdirectory(contrib)

if(APQ_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
