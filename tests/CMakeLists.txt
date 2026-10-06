add_executable(cymatica_tests
    unit/test_smoke.cpp
    unit/test_random.cpp
    unit/test_music_clock.cpp
    unit/test_fixed_step.cpp
    unit/test_exchange.cpp
    unit/test_run_record.cpp
)

target_link_libraries(cymatica_tests
    PRIVATE
        Catch2::Catch2WithMain
        cymatica_core
        cymatica_audio
        cymatica_replay
        cymatica_compile_options
)

include(${catch2_SOURCE_DIR}/extras/Catch.cmake)
# Catch tags become CTest labels, e.g. [device] -> label "device".
catch_discover_tests(cymatica_tests ADD_TAGS_AS_LABELS)
