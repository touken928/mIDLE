# Patch imtui-impl-ncurses.cpp so that ANSI color 16 (pure black)
# uses the terminal's default background color (-1) instead.
# This makes windows like the editor blend with the terminal.

set(_patch_file "${CMAKE_CURRENT_SOURCE_DIR}/third_party/imtui/src/imtui-impl-ncurses.cpp")
set(_patch_marker "${CMAKE_CURRENT_BINARY_DIR}/.imtui_ncurses_patched")

if(NOT EXISTS "${_patch_marker}")
    file(READ "${_patch_file}" _contents)
    string(REPLACE "init_pair(nColPairs, f, b);"
        "short b_init = (b == 16) ? -1 : (short)b;\n                init_pair(nColPairs, f, b_init);"
        _patched
        "${_contents}"
    )
    file(WRITE "${_patch_file}" "${_patched}")
    file(WRITE "${_patch_marker}" "")
    message(STATUS "[imtui] patched ncurses renderer for terminal-default black background")
endif()
