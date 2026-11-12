# Diagnostic audio tools that open a device of their own. Kept out of cymatica_audio so that the engine library
# has a single device owner (docs/dependencies.md, CONS-01). Only device_probe_v1 in cymatica_game uses it.
add_library(cymatica_audio_diag STATIC
    endpoint_volume.cpp
    endpoint_volume.h
    loopback_capture.cpp
    loopback_capture.h
)

target_include_directories(cymatica_audio_diag PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# Declarations only: MINIAUDIO_IMPLEMENTATION stays in engine/audio/audio_engine.cpp.
target_include_directories(cymatica_audio_diag SYSTEM PRIVATE ${PROJECT_SOURCE_DIR}/third_party/miniaudio)
target_link_libraries(cymatica_audio_diag PUBLIC cymatica_audio)
target_link_libraries(cymatica_audio_diag PRIVATE cymatica_compile_options)
target_compile_features(cymatica_audio_diag PUBLIC cxx_std_20)
if(WIN32)
    # Core Audio endpoint volume, read only (CP-1, R-M25-12): ole32 for COM, part of the Windows SDK
    target_link_libraries(cymatica_audio_diag PRIVATE ole32)
endif()
