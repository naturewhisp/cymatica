add_executable(cymatica_tests
    unit/test_smoke.cpp
    unit/test_random.cpp
    unit/test_music_clock.cpp
    unit/test_fixed_step.cpp
    unit/test_exchange.cpp
    unit/test_run_record.cpp
    unit/test_input_dispatcher.cpp
    unit/test_audio_probe.cpp
    unit/test_scheduler.cpp
    unit/test_voice_allocator.cpp
    unit/test_synth_dsp.cpp
    unit/test_music_planner.cpp
    unit/test_realtime_profile.cpp
    unit/test_event_chain.cpp
    unit/test_cacp_analysis.cpp
    unit/test_stem_analysis.cpp
    unit/test_evaluation_pack.cpp
)

target_link_libraries(cymatica_tests
    PRIVATE
        Catch2::Catch2WithMain
        cymatica_core
        cymatica_audio
        cymatica_cacp
        cymatica_replay
        nlohmann_json::nlohmann_json
        cymatica_compile_options
)

include(${catch2_SOURCE_DIR}/extras/Catch.cmake)
# Catch tags become CTest labels, e.g. [device] -> label "device".
catch_discover_tests(cymatica_tests ADD_TAGS_AS_LABELS)
