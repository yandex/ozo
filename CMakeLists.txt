cmake_minimum_required(VERSION 3.12)

project(ozo VERSION 0.0.1)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

# note: boost::coroutine should automatically pull in all the dependencies,
#       however, due to changes in boost it doesn't always work until CMake
#       is updated. we therefore look for all the dependencies ourselves.
find_package(Boost COMPONENTS coroutine context system thread atomic REQUIRED)
find_package(PostgreSQL REQUIRED)

# Try and find provided resource pool (e.g. from conan), default to vendored version otherwise
find_package(resource_pool 0.1.0 QUIET)
if (NOT resource_pool_FOUND)
    add_subdirectory(contrib)
endif()

option(OZO_BUILD_TESTS "Enable tests build" OFF)
option(OZO_COVERAGE "Enable tests coverage" OFF)
option(OZO_BUILD_EXAMPLES "Enable examples build" OFF)

set(CMAKE_CXX_EXTENSIONS OFF)

add_library(ozo INTERFACE)
add_library(yandex::ozo ALIAS ozo)

target_compile_features(ozo INTERFACE cxx_std_17)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
    # disable warnings about a compiler-specific option used for gnu-compatible compilers
    target_compile_options(ozo INTERFACE -Wno-gnu-string-literal-operator-template -Wno-gnu-zero-variadic-macro-arguments)
endif()


target_include_directories(ozo INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_definitions(ozo INTERFACE -DBOOST_COROUTINES_NO_DEPRECATION_WARNING)
target_compile_definitions(ozo INTERFACE -DBOOST_HANA_CONFIG_ENABLE_STRING_UDL)

target_link_libraries(ozo INTERFACE Boost::coroutine)
target_link_libraries(ozo INTERFACE PostgreSQL::PostgreSQL)
target_link_libraries(ozo INTERFACE elsid::resource_pool)

install(
    DIRECTORY   include/ozo
    DESTINATION include
)

install(
    TARGETS     ozo
    EXPORT      ozo-targets
    DESTINATION lib
)

install(
    EXPORT      ozo-targets
    NAMESPACE   yandex::
    DESTINATION lib/cmake/ozo
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/ozo/ozo-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

export(
    EXPORT ozo-targets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/ozo/ozo-targets.cmake"
    NAMESPACE yandex::
)

configure_file(cmake/ozo-config.cmake
    "${CMAKE_CURRENT_BINARY_DIR}/ozo/ozo-config.cmake"
    COPYONLY
)

install(
    FILES
        cmake/ozo-config.cmake
        cmake/modules/FindPostgreSQL.cmake
        "${CMAKE_CURRENT_BINARY_DIR}/ozo/ozo-config-version.cmake"
    DESTINATION
        lib/cmake/ozo
)

if(OZO_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(OZO_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(OZO_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()
