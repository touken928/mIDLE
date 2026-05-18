# ═══════════════════════════════════════════════════════════════
#  mpy — self-contained MicroPython embedding target
# ═══════════════════════════════════════════════════════════════

set(MP_TOP       "${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/micropython")
set(MP_EMBED_DIR "${CMAKE_CURRENT_BINARY_DIR}/micropython_embed")

find_program(MPY_MAKE_PROGRAM NAMES gmake make mingw32-make REQUIRED)
find_program(MPY_PYTHON_PROGRAM NAMES python3 python py REQUIRED)

file(RELATIVE_PATH MP_TOP_MAKE "${CMAKE_CURRENT_SOURCE_DIR}" "${MP_TOP}")
file(RELATIVE_PATH MP_EMBED_DIR_MAKE "${CMAKE_CURRENT_SOURCE_DIR}" "${MP_EMBED_DIR}")
file(RELATIVE_PATH MP_EMBED_BUILD_MAKE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}/build-embed"
)
file(TO_CMAKE_PATH "${CMAKE_C_COMPILER}" MPY_CC)
file(TO_CMAKE_PATH "${MP_TOP}" MP_TOP_CMAKE)
file(TO_CMAKE_PATH "${MP_EMBED_DIR}" MP_EMBED_DIR_CMAKE)
file(TO_CMAKE_PATH "${CMAKE_CURRENT_SOURCE_DIR}" MPY_PORT_DIR_CMAKE)

# ── 1.  Generate embed package (once) ───────────────────────
if(NOT EXISTS "${MP_EMBED_DIR}/py/compile.c")
    message(STATUS "[mpy] generating embed package …")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/gen_embed.mk"
        "MICROPYTHON_TOP = ${MP_TOP_MAKE}\n"
        "PACKAGE_DIR    = ${MP_EMBED_DIR_MAKE}\n"
        "include ${MP_TOP_MAKE}/ports/embed/embed.mk\n"
    )
    # Run from src/mpy/ so mpconfigport.h is found as a direct dependency.
    # BUILD is passed on the command line (highest precedence in GNU make)
    # to prevent embed.mk's default BUILD=build-embed leaking into the source tree.
    execute_process(
        COMMAND "${MPY_MAKE_PROGRAM}" -f "${CMAKE_CURRENT_BINARY_DIR}/gen_embed.mk"
            BUILD="${MP_EMBED_BUILD_MAKE}"
            PYTHON="${MPY_PYTHON_PROGRAM}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        RESULT_VARIABLE _res
    )
    if(NOT _res EQUAL 0)
        message(FATAL_ERROR "[mpy] embed package generation failed")
    endif()
endif()

message(STATUS "[mpy] regenerating headers")
execute_process(
    COMMAND "${MPY_PYTHON_PROGRAM}" "${CMAKE_CURRENT_SOURCE_DIR}/regen_mpy_headers.py"
        --cc "${MPY_CC}"
        --python "${MPY_PYTHON_PROGRAM}"
        --mp-top "${MP_TOP_CMAKE}"
        --embed-dir "${MP_EMBED_DIR_CMAKE}"
        --port-dir "${MPY_PORT_DIR_CMAKE}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    RESULT_VARIABLE _regen_res
)
if(NOT _regen_res EQUAL 0)
    message(FATAL_ERROR "[mpy] header regeneration failed")
endif()

# ── 2.  Sources ─────────────────────────────────────────────
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

# ── 3.  mpy library (C + C++) ───────────────────────────────
add_library(mpy STATIC
    ${EMBED_C_SOURCES}
    ${EMBED_EXTMOD_SOURCES}
    mpyhalport.c                # HAL (output capture)
    mpy.cpp                     # C++ wrapper
)

target_include_directories(mpy PUBLIC
    "${MP_TOP}"                             # shared/readline/readline.h
    "${MP_EMBED_DIR}"                       # py, extmod, shared
    "${MP_EMBED_DIR}/port"                  # micropython_embed.h
    "${MP_EMBED_DIR}/genhdr"                # qstrdefs.generated.h
    "${MP_EMBED_DIR}/py"                    # py headers
    "${MP_TOP}/extmod"                      # extmod headers (e.g. modtime.h)
    "${CMAKE_CURRENT_SOURCE_DIR}"           # mpconfigport.h, mpyhalport.h, mpy.h
)

target_compile_options(mpy PRIVATE -w)

find_package(Threads REQUIRED)
target_link_libraries(mpy PRIVATE Threads::Threads)
