# ═══════════════════════════════════════════════════════════════
#  Python (MicroPython embed)
# ═══════════════════════════════════════════════════════════════

set(MP_TOP       "${CMAKE_CURRENT_SOURCE_DIR}/../../../third_party/micropython")
set(MP_EMBED_DIR "${CMAKE_CURRENT_BINARY_DIR}/micropython_embed")
set(PYTHON_CONFIG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/config")

find_program(MPY_MAKE_PROGRAM NAMES gmake make mingw32-make REQUIRED)
find_program(MPY_PYTHON_PROGRAM NAMES python3 python py REQUIRED)

file(RELATIVE_PATH MP_TOP_MAKE "${PYTHON_CONFIG_DIR}" "${MP_TOP}")
file(RELATIVE_PATH MP_EMBED_DIR_MAKE "${PYTHON_CONFIG_DIR}" "${MP_EMBED_DIR}")
file(RELATIVE_PATH MP_EMBED_BUILD_MAKE
    "${PYTHON_CONFIG_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}/build-embed"
)
file(TO_CMAKE_PATH "${CMAKE_C_COMPILER}" MPY_CC)
file(TO_CMAKE_PATH "${MP_TOP}" MP_TOP_CMAKE)
file(TO_CMAKE_PATH "${MP_EMBED_DIR}" MP_EMBED_DIR_CMAKE)
file(TO_CMAKE_PATH "${PYTHON_CONFIG_DIR}" MPY_PORT_DIR_CMAKE)

if(NOT EXISTS "${MP_EMBED_DIR}/py/compile.c")
    message(STATUS "[python] generating embed package …")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/gen_embed.mk"
        "MICROPYTHON_TOP = ${MP_TOP_MAKE}\n"
        "PACKAGE_DIR    = ${MP_EMBED_DIR_MAKE}\n"
        "include ${MP_TOP_MAKE}/ports/embed/embed.mk\n"
    )
    execute_process(
        COMMAND "${MPY_MAKE_PROGRAM}" -f "${CMAKE_CURRENT_BINARY_DIR}/gen_embed.mk"
            BUILD="${MP_EMBED_BUILD_MAKE}"
            PYTHON="${MPY_PYTHON_PROGRAM}"
        WORKING_DIRECTORY "${PYTHON_CONFIG_DIR}"
        RESULT_VARIABLE _res
    )
    if(NOT _res EQUAL 0)
        message(FATAL_ERROR "[python] embed package generation failed")
    endif()
endif()

message(STATUS "[python] regenerating headers")
execute_process(
    COMMAND "${MPY_PYTHON_PROGRAM}" "${CMAKE_CURRENT_SOURCE_DIR}/regen_embed_headers.py"
        --cc "${MPY_CC}"
        --python "${MPY_PYTHON_PROGRAM}"
        --mp-top "${MP_TOP_CMAKE}"
        --embed-dir "${MP_EMBED_DIR_CMAKE}"
        --port-dir "${MPY_PORT_DIR_CMAKE}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    RESULT_VARIABLE _regen_res
)
if(NOT _regen_res EQUAL 0)
    message(FATAL_ERROR "[python] header regeneration failed")
endif()

file(GLOB_RECURSE EMBED_C_SOURCES
    "${MP_EMBED_DIR}/py/*.c"
    "${MP_EMBED_DIR}/shared/runtime/*.c"
    "${MP_EMBED_DIR}/port/embed_util.c"
)

file(GLOB EMBED_EXTMOD_SOURCES
    "${MP_TOP}/extmod/modjson.c"
    "${MP_TOP}/extmod/modrandom.c"
    "${MP_TOP}/extmod/modheapq.c"
    "${MP_TOP}/extmod/modbinascii.c"
    "${MP_TOP}/extmod/modhashlib.c"
)

add_library(midle_lang_python STATIC
    ${EMBED_C_SOURCES}
    ${EMBED_EXTMOD_SOURCES}
    hal/port.c
    backend.cpp
    module.cpp
    highlight/tokenizer.cpp
)

target_include_directories(midle_lang_python PUBLIC
    "${MP_TOP}"
    "${MP_EMBED_DIR}"
    "${MP_EMBED_DIR}/port"
    "${MP_EMBED_DIR}/genhdr"
    "${MP_EMBED_DIR}/py"
    "${MP_TOP}/extmod"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/config"
    "${CMAKE_CURRENT_SOURCE_DIR}/hal"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../"
)

target_compile_options(midle_lang_python PRIVATE -w)

find_package(Threads REQUIRED)
target_link_libraries(midle_lang_python PUBLIC midle_runtime_core Threads::Threads)
