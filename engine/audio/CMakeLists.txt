add_library(cymatica_audio STATIC
    audio_engine.cpp
    audio_engine.h
)

target_include_directories(cymatica_audio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# miniaudio is private: no other target sees or implements it.
target_include_directories(cymatica_audio SYSTEM PRIVATE ${PROJECT_SOURCE_DIR}/third_party/miniaudio)
target_link_libraries(cymatica_audio PRIVATE cymatica_compile_options)
target_compile_features(cymatica_audio PUBLIC cxx_std_20)
