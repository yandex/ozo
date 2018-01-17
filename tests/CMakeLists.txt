set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -Werror -Wno-ignored-optimization-argument -fno-devirtualize")

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
    connection_pool.cpp
    type_traits.cpp
)

set(LIBRARIES
    pthread
    gmock_main
    gherkin-cpp
    ${Boost_LIBRARIES}
)

add_executable(apq_tests ${SOURCES})
add_dependencies(apq_tests GUnit)
target_link_libraries(apq_tests ${LIBRARIES})
add_test(apq_tests apq_tests)

if(APQ_COVERAGE AND CMAKE_COMPILER_IS_GNUCXX)
    include(CodeCoverage)
    APPEND_COVERAGE_COMPILER_FLAGS()
    set(COVERAGE_EXCLUDES 'tests/*' 'contrib/*' 'boost/*' 'c++/*')
    SETUP_TARGET_FOR_COVERAGE(
        NAME apq_coverage
        EXECUTABLE apq_tests
        DEPENDENCIES apq_tests
    )
endif()
