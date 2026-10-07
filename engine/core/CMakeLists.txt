add_library(cymatica_core STATIC
    fixed_step.cpp
    fixed_step.h
    unsigned_math.h
    random_key.h
    stable_random.cpp
    stable_random.h
    version_ids.h
)

target_include_directories(cymatica_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(cymatica_core PUBLIC
    cymatica_compile_options
)
