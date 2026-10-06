cmake_minimum_required(VERSION 3.25)
project(CYMATICA LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Spec section 29.3 build options (tools/lab are not implemented in M0)
option(CYMATICA_BUILD_GAME "Build game runtime" ON)
option(CYMATICA_BUILD_TESTS "Build tests" ON)

# Project-wide compile settings for CYMATICA targets only (not third-party code)
add_library(cymatica_compile_options INTERFACE)
if(MSVC)
    target_compile_options(cymatica_compile_options INTERFACE /utf-8 /W4 /permissive-)
else()
    target_compile_options(cymatica_compile_options INTERFACE -Wall -Wextra -Wpedantic)
endif()

include(FetchContent)

# -----------------------------------------------------------------------------
# raylib 6.0 (ADR-0001). Single source of truth for its options: keep them here,
# not in CMakePresets.json.
#  - SUPPORT_MODULE_RAUDIO=OFF: CYMATICA has a single miniaudio owner.
#  - SUPPORT_CUSTOM_FRAME_CONTROL=OFF: CUSTOMIZE_BUILD=ON would otherwise enable it,
#    and EndDrawing() would neither swap buffers nor pace frames.
# -----------------------------------------------------------------------------
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(CUSTOMIZE_BUILD ON CACHE BOOL "" FORCE)
set(SUPPORT_MODULE_RAUDIO OFF CACHE BOOL "" FORCE)
set(SUPPORT_CUSTOM_FRAME_CONTROL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG dbc56a87da87d973a9c5baa4e7438a9d20121d28 # 6.0
    GIT_SHALLOW FALSE
)
FetchContent_MakeAvailable(raylib)

# Catch2 v3.16.0 (ADR-0001)
if(CYMATICA_BUILD_TESTS)
    FetchContent_Declare(Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG 317ac1ed4c0bb6e6b91eafc817e05c488feffcb3 # v3.16.0
        GIT_SHALLOW FALSE
    )
    FetchContent_MakeAvailable(Catch2)
endif()

# nlohmann/json v3.11.3 (ADR-0002)
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
set(JSON_Install OFF CACHE BOOL "" FORCE)
FetchContent_Declare(nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG 9cca280a4d0ccf0c08f47a99aa71d1b0e52f8d03 # v3.11.3
    GIT_SHALLOW FALSE
)
FetchContent_MakeAvailable(nlohmann_json)

# Engine modules (Spec §21)
add_subdirectory(engine/core)
add_subdirectory(engine/audio)
add_subdirectory(engine/replay)

if(CYMATICA_BUILD_GAME)
    add_subdirectory(apps/cymatica_game)
endif()

if(CYMATICA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
