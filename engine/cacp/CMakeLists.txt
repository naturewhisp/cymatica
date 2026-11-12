# Offline CACP analysis tools (ADR-0006 D-M2-14, R5 option T3): descriptors, PNG, hashes and analysis scenarios.
# Never linked into the realtime audio path: it depends on cymatica_audio, not the other way round.
add_library(cymatica_cacp STATIC
    acao02_calibration.cpp
    acao02_calibration.h
    check_selftest.cpp
    check_selftest.h
    analysis_status.h
    diversity.cpp
    diversity.h
    ensemble_tools.cpp
    ensemble_tools.h
    evaluation_pack.cpp
    evaluation_pack.h
    fft.cpp
    fft.h
    loudness.cpp
    loudness.h
    m2_suite.cpp
    m2_suite.h
    mel.cpp
    mel.h
    onset.cpp
    onset.h
    onset_parity.cpp
    onset_parity.h
    png_writer.cpp
    png_writer.h
    presence_stats.cpp
    presence_stats.h
    procedural_scenario.cpp
    procedural_scenario.h
    release_ab.cpp
    release_ab.h
    sha256.cpp
    sha256.h
    stereo.cpp
    stem_analysis.cpp
    stem_analysis.h
    stereo.h
    stft.cpp
    stft.h
    staff_view.cpp
    staff_view.h
    trama_diagnosis.cpp
    trama_diagnosis.h
)

target_include_directories(cymatica_cacp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(cymatica_cacp PUBLIC cymatica_audio cymatica_core)
# nlohmann/json (pinned, ADR-0002) reads and writes the evaluation pack truth and results (D-M2-15, Inc-5 5d)
target_link_libraries(cymatica_cacp PRIVATE cymatica_compile_options nlohmann_json::nlohmann_json)
target_compile_features(cymatica_cacp PUBLIC cxx_std_20)
