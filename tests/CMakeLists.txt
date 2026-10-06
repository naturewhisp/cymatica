add_executable(cymatica_tests
    unit/test_smoke.cpp
)

target_link_libraries(cymatica_tests
    PRIVATE
        Catch2::Catch2WithMain
        cymatica_audio
        cymatica_compile_options
)

include(${catch2_SOURCE_DIR}/extras/Catch.cmake)
# Catch tags become CTest labels, e.g. [device] -> label "device".
catch_discover_tests(cymatica_tests ADD_TAGS_AS_LABELS)
