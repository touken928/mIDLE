# ═══════════════════════════════════════════════════════════════
#  mpy — self-contained MicroPython embedding target
# ═══════════════════════════════════════════════════════════════

set(MP_TOP       "${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/micropython")
set(MP_EMBED_DIR "${CMAKE_CURRENT_BINARY_DIR}/micropython_embed")

# ── 1.  Generate embed package (once) ───────────────────────
if(NOT EXISTS "${MP_EMBED_DIR}/py/compile.c")
    message(STATUS "[mpy] generating embed package …")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/gen_embed.mk"
        "MICROPYTHON_TOP = ${MP_TOP}\n"
        "PACKAGE_DIR    = ${MP_EMBED_DIR}\n"
        "include ${MP_TOP}/ports/embed/embed.mk\n"
    )
    # Run from src/mpy/ so mpconfigport.h is found as a direct dependency.
    # BUILD is passed on the command line (highest precedence in GNU make)
    # to prevent embed.mk's default BUILD=build-embed leaking into the source tree.
    execute_process(
        COMMAND ${CMAKE_MAKE_PROGRAM} -f "${CMAKE_CURRENT_BINARY_DIR}/gen_embed.mk"
            BUILD="${CMAKE_CURRENT_BINARY_DIR}/build-embed"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        RESULT_VARIABLE _res
    )
    if(NOT _res EQUAL 0)
        message(FATAL_ERROR "[mpy] embed package generation failed")
    endif()
endif()

# ── 2.  Sources ─────────────────────────────────────────────
file(GLOB_RECURSE EMBED_C_SOURCES
    "${MP_EMBED_DIR}/py/*.c"
    "${MP_EMBED_DIR}/shared/runtime/*.c"
    "${MP_EMBED_DIR}/port/embed_util.c"
)

# ── 3.  mpy library (C + C++) ───────────────────────────────
add_library(mpy STATIC
    ${EMBED_C_SOURCES}
    mpyhalport.c                # HAL (output capture)
    mpy.cpp                     # C++ wrapper
)

target_include_directories(mpy PUBLIC
    "${MP_EMBED_DIR}"                       # py, extmod, shared
    "${MP_EMBED_DIR}/port"                  # micropython_embed.h
    "${MP_EMBED_DIR}/genhdr"                # qstrdefs.generated.h
    "${MP_EMBED_DIR}/py"                    # py headers
    "${CMAKE_CURRENT_SOURCE_DIR}"           # mpconfigport.h, mpyhalport.h, mpy.h
)

target_compile_options(mpy PRIVATE -w)

find_package(Threads REQUIRED)
target_link_libraries(mpy PRIVATE Threads::Threads)
