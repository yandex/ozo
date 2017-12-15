set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -Werror -fno-devirtualize")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-ignored-optimization-argument")
endif()

find_package(Boost COMPONENTS system thread REQUIRED)
include_directories(SYSTEM "${Boost_INCLUDE_DIRS}")

find_package(PostgreSQL)
include_directories(SYSTEM "${PostgreSQL_INCLUDE_DIRS}")

include(ExternalProject)
ExternalProject_Add(
    GUnit
    GIT_REPOSITORY "https://github.com/elsid/GUnit.git"
    GIT_TAG dev
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}
    UPDATE_COMMAND ""
    LOG_DOWNLOAD ON
    LOG_CONFIGURE ON
    LOG_BUILD ON
)
ExternalProject_Get_Property(GUnit source_dir)
include_directories(SYSTEM "${source_dir}/include")
include_directories(SYSTEM "${CMAKE_CURRENT_BINARY_DIR}/include")
link_directories("${CMAKE_CURRENT_BINARY_DIR}/lib")

include_directories(SYSTEM "${source_dir}/libs/gherkin-cpp/include")
include_directories(SYSTEM "${source_dir}/libs/json/src")
link_directories("${source_dir}/libs/gherkin-cpp")

set(SOURCES
    async_connect.cpp
    binary_query.cpp
    binary_serialization.cpp
    bind.cpp
    connection.cpp
    connection_info.cpp
    connection_pool.cpp
    query_builder.cpp
    type_traits.cpp
)

set(LIBRARIES
    pthread
    gmock_main
    gherkin-cpp
    ${Boost_LIBRARIES}
    ${PostgreSQL_LIBRARIES}
)

add_executable(ozo_tests ${SOURCES})
add_dependencies(ozo_tests GUnit)
target_link_libraries(ozo_tests ${LIBRARIES})
add_test(ozo_tests ozo_tests)

if(OZO_COVERAGE AND CMAKE_COMPILER_IS_GNUCXX)
    include(CodeCoverage)
    APPEND_COVERAGE_COMPILER_FLAGS()
    set(COVERAGE_EXCLUDES 'tests/*' 'contrib/*' 'boost/*' 'c++/*')
    SETUP_TARGET_FOR_COVERAGE(
        NAME ozo_coverage
        EXECUTABLE ozo_tests
        DEPENDENCIES ozo_tests
    )
endif()
