add_library(cymatica_replay STATIC
    run_record.cpp
    run_record.h
    seed_bank.cpp
    seed_bank.h
)

target_include_directories(cymatica_replay PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(cymatica_replay PUBLIC
    cymatica_core
    nlohmann_json::nlohmann_json
    cymatica_compile_options
)
