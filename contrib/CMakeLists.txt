add_subdirectory(resource_pool)

if(OZO_BUILD_BENCHMARKS)
    set(JSON_BuildTests OFF)
    set(JSON_Install OFF)
    set(JSON_MultipleHeaders OFF)
    add_subdirectory(nlohmann_json)
endif()
