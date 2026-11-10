# Single audio owner (spec 28.2, docs/dependencies.md, CONS-01). Fails if:
#  - any source of cymatica_audio other than audio_engine.cpp includes miniaudio or opens a device;
#  - MINIAUDIO_IMPLEMENTATION is defined anywhere but audio_engine.cpp;
#  - miniaudio is included outside audio_engine.cpp and the diagnostic capture of cymatica_audio_diag.
# Run by CTest: cmake -DROOT=<repository root> -P check_single_audio_owner.cmake
if(NOT DEFINED ROOT)
    message(FATAL_ERROR "ROOT not set")
endif()

set(OWNER "engine/audio/audio_engine.cpp")
set(DIAGNOSTIC "engine/audio_diag/loopback_capture.cpp")

file(GLOB_RECURSE SOURCES RELATIVE "${ROOT}"
    "${ROOT}/engine/*.cpp" "${ROOT}/engine/*.h"
    "${ROOT}/apps/*.cpp" "${ROOT}/apps/*.h"
    "${ROOT}/tests/*.cpp" "${ROOT}/tests/*.h")
list(LENGTH SOURCES SOURCE_COUNT)
if(SOURCE_COUNT LESS 20)
    message(FATAL_ERROR "only ${SOURCE_COUNT} sources found under ${ROOT}: wrong ROOT, nothing was checked")
endif()
foreach(REQUIRED "${OWNER}" "${DIAGNOSTIC}")
    if(NOT EXISTS "${ROOT}/${REQUIRED}")
        message(FATAL_ERROR "${REQUIRED} not found: update this check together with the file layout")
    endif()
endforeach()

set(FAILURES "")
set(IMPLEMENTATIONS 0)
foreach(SOURCE IN LISTS SOURCES)
    file(READ "${ROOT}/${SOURCE}" TEXT)
    string(REGEX MATCH "#[ \t]*define[ \t]+MINIAUDIO_IMPLEMENTATION" HAS_IMPLEMENTATION "${TEXT}")
    string(REGEX MATCH "#[ \t]*include[ \t]*[\"<][^\">]*miniaudio\\.h[\">]" HAS_INCLUDE "${TEXT}")
    string(REGEX MATCH "ma_device_init|ma_device_config_init|ma_context_init" HAS_DEVICE "${TEXT}")
    if(HAS_IMPLEMENTATION)
        math(EXPR IMPLEMENTATIONS "${IMPLEMENTATIONS} + 1")
        if(NOT SOURCE STREQUAL OWNER)
            list(APPEND FAILURES "${SOURCE}: defines MINIAUDIO_IMPLEMENTATION")
        endif()
    endif()
    if((HAS_INCLUDE OR HAS_DEVICE) AND NOT SOURCE STREQUAL OWNER AND NOT SOURCE STREQUAL DIAGNOSTIC)
        list(APPEND FAILURES "${SOURCE}: includes miniaudio or opens a device outside the owner and the diagnostic capture")
    endif()
endforeach()
if(NOT IMPLEMENTATIONS EQUAL 1)
    list(APPEND FAILURES "MINIAUDIO_IMPLEMENTATION defined ${IMPLEMENTATIONS} times, expected exactly 1")
endif()

# The diagnostic capture must stay out of the engine library.
file(READ "${ROOT}/engine/audio/CMakeLists.txt" AUDIO_CMAKE)
if(AUDIO_CMAKE MATCHES "loopback_capture|audio_diag")
    list(APPEND FAILURES "engine/audio/CMakeLists.txt: cymatica_audio references the diagnostic capture")
endif()

if(FAILURES)
    list(JOIN FAILURES "\n  " REPORT)
    message(FATAL_ERROR "single audio owner violated:\n  ${REPORT}")
endif()
message(STATUS "single audio owner: ${SOURCE_COUNT} sources checked, 1 implementation, 0 violations")
