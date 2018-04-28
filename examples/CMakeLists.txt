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
target_link_libraries(ozo_request ${LIBRARIES})
