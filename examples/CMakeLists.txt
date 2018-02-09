set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -Werror")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-gnu-string-literal-operator-template")
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

add_executable(ozo_query_builder query_builder.cpp)
target_link_libraries(ozo_query_builder ${LIBRARIES})

add_executable(ozo_query_conf query_conf.cpp)
target_link_libraries(ozo_query_conf ${LIBRARIES})
