set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wsign-compare -pedantic -Werror")

execute_process(COMMAND echo "int main() { return 0;}" OUTPUT_FILE test.cpp)
execute_process(COMMAND ${CMAKE_CXX_COMPILER} -std=c++17 test.cpp -o a.out RESULT_VARIABLE CPP17_READY_RESULT)
execute_process(COMMAND rm test.cpp OUTPUT_QUIET ERROR_QUIET)
execute_process(COMMAND rm a.out OUTPUT_QUIET ERROR_QUIET)

if ("${CPP17_READY_RESULT}" STREQUAL "0")
    set(CPP17_READY 1)
else()
    set(CPP17_NOT_READY 1)
endif()

if (CPP17_READY)
    message(STATUS "compiler supports -std=c++17")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
else()
    message(STATUS "compiler does not support -std=c++17, using -std=c++1z instead")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z")
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-ignored-optimization-argument")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-gnu-string-literal-operator-template")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-gnu-zero-variadic-macro-arguments")
endif()

find_program(CCACHE_FOUND ccache)

if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif()

find_package(Boost COMPONENTS coroutine context system thread REQUIRED)
include_directories(SYSTEM "${Boost_INCLUDE_DIRS}")

find_package(PostgreSQL)
include_directories(SYSTEM "${PostgreSQL_INCLUDE_DIRS}")

include(ExternalProject)
ExternalProject_Add(
    GoogleTest
    GIT_REPOSITORY "https://github.com/google/googletest.git"
    GIT_TAG release-1.8.0
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}
    UPDATE_COMMAND ""
    LOG_DOWNLOAD ON
    LOG_CONFIGURE ON
    LOG_BUILD ON
)
ExternalProject_Get_Property(GoogleTest source_dir)
include_directories(SYSTEM "${source_dir}/include")
include_directories(SYSTEM "${CMAKE_CURRENT_BINARY_DIR}/include")
link_directories("${CMAKE_CURRENT_BINARY_DIR}/lib")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

set(SOURCES
    impl/async_connect.cpp
    binary_deserialization.cpp
    binary_query.cpp
    binary_serialization.cpp
    bind.cpp
    connection.cpp
    connection_info.cpp
    connection_pool.cpp
    query_builder.cpp
    query_conf.cpp
    type_traits.cpp
    concept.cpp
    result.cpp
    impl/async_send_query_params.cpp
    impl/async_get_result.cpp
    detail/base36.cpp
    impl/request_oid_map.cpp
    impl/request_oid_map_handler.cpp
    main.cpp
)

if(OZO_BUILD_PG_TESTS)
    set(SOURCES
        ${SOURCES}
        result_integration.cpp
        request_integration.cpp
        get_connection_integration.cpp
        execute_integration.cpp
    )
    add_definitions(-DOZO_PG_TEST_CONNINFO="${OZO_PG_TEST_CONNINFO}")
endif()

set(LIBRARIES
    pthread
    gmock
    ${Boost_LIBRARIES}
    ${PostgreSQL_LIBRARIES}
)

add_executable(ozo_tests ${SOURCES})
add_dependencies(ozo_tests GoogleTest)
target_link_libraries(ozo_tests ${LIBRARIES})
add_test(ozo_tests ozo_tests)

if(OZO_COVERAGE AND CMAKE_COMPILER_IS_GNUCXX)
    include(CodeCoverage)
    APPEND_COVERAGE_COMPILER_FLAGS()
    set(COVERAGE_EXCLUDES "'.*/(tests|contrib|gmock|gtest)/.*'")
    SETUP_TARGET_FOR_COVERAGE_GCOVR(
        NAME ozo_coverage
        EXECUTABLE ctest
        DEPENDENCIES ozo_tests
    )
endif()
