# Patched copy in build/generated only — third_party is never modified.

set(_src "${CMAKE_CURRENT_SOURCE_DIR}/third_party/imtui/src/imtui-impl-ncurses.cpp")
set(_out "${CMAKE_CURRENT_BINARY_DIR}/generated/imtui-impl-ncurses.cpp")

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(READ "${_src}" _contents)
string(REPLACE
    "init_pair(nColPairs, f, b);"
    "short b_init = (b == 16) ? -1 : (short)b;\n                init_pair(nColPairs, f, b_init);"
    _patched
    "${_contents}"
)
file(WRITE "${_out}" "${_patched}")
set(IMTUI_NCURSES_PATCHED_CPP "${_out}" CACHE INTERNAL "")

message(STATUS "[imtui] patched ncurses renderer: ${IMTUI_NCURSES_PATCHED_CPP}")
