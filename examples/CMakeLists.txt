set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wsign-compare -pedantic -Werror")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-ignored-optimization-argument")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-gnu-string-literal-operator-template")
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

set(LIBRARIES
    pthread
    ${Boost_LIBRARIES}
    ${PostgreSQL_LIBRARIES}
)

add_executable(ozo_request request.cpp)
target_compile_features(ozo_request PRIVATE cxx_std_17)
target_link_libraries(ozo_request ${LIBRARIES})

add_executable(ozo_connection_pool connection_pool.cpp)
target_compile_features(ozo_connection_pool PRIVATE cxx_std_17)
target_link_libraries(ozo_connection_pool ${LIBRARIES})
