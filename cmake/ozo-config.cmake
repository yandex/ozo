include(CMakeFindDependencyMacro)

# Find PostgreSQL using new provided script
# Remove this once we move to CMake 3.14+
set(CMAKE_MODULE_PATH_old_ ${CMAKE_MODULE_PATH})
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}")
find_dependency(PostgreSQL)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH_old_})

find_dependency(Boost COMPONENTS coroutine context system thread atomic)
find_dependency(resource_pool)

include("${CMAKE_CURRENT_LIST_DIR}/ozo-targets.cmake")
