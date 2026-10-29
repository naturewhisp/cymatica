# Offline CACP analysis tools (ADR-0006 D-M2-14, R5 option T3): descriptors, PNG, hashes and analysis scenarios.
# Never linked into the realtime audio path: it depends on cymatica_audio, not the other way round.
add_library(cymatica_cacp STATIC
    acao02_calibration.cpp
    acao02_calibration.h
    analysis_status.h
    fft.cpp
    fft.h
    loudness.cpp
    loudness.h
    mel.cpp
    mel.h
    onset.cpp
    onset.h
    onset_parity.cpp
    onset_parity.h
    png_writer.cpp
    png_writer.h
    sha256.cpp
    sha256.h
    stereo.cpp
    stem_analysis.cpp
    stem_analysis.h
    stereo.h
    stft.cpp
    stft.h
)

target_include_directories(cymatica_cacp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(cymatica_cacp PUBLIC cymatica_audio cymatica_core)
target_link_libraries(cymatica_cacp PRIVATE cymatica_compile_options)
target_compile_features(cymatica_cacp PUBLIC cxx_std_20)
