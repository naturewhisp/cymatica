add_library(cymatica_audio STATIC
    audio_control.h
    audio_engine.cpp
    audio_engine.h
    audio_probe.cpp
    audio_probe.h
    audio_scheduler.cpp
    audio_scheduler.h
    audio_telemetry.h
    music_clock.cpp
    music_clock.h
    music_intent.h
    music_planner.cpp
    music_planner.h
    music_position.h
    master_limiter.cpp
    master_limiter.h
    realtime_exchange.h
    role_bus_processor.cpp
    role_bus_processor.h
    synth_dsp.h
    voice_allocator.cpp
    voice_allocator.h
)

target_include_directories(cymatica_audio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# miniaudio is private: no other target sees or implements it.
target_include_directories(cymatica_audio SYSTEM PRIVATE ${PROJECT_SOURCE_DIR}/third_party/miniaudio)
target_link_libraries(cymatica_audio PUBLIC cymatica_core)
target_link_libraries(cymatica_audio PRIVATE cymatica_compile_options)
target_compile_features(cymatica_audio PUBLIC cxx_std_20)
