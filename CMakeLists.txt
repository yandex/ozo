cmake_minimum_required(VERSION 2.8)

project(ozo)

option(OZO_BUILD_TESTS "Enable tests build" OFF)
option(OZO_COVERAGE "Enable tests coverage" OFF)
option(OZO_BUILD_EXAMPLES "Enable examples build" OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBOOST_COROUTINES_NO_DEPRECATION_WARNING")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBOOST_HANA_CONFIG_ENABLE_STRING_UDL")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include_directories(contrib/resource_pool/include)
include_directories(include)

add_subdirectory(contrib)

if(OZO_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(OZO_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
