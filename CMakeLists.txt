cmake_minimum_required(VERSION 3.16)
project(mIDLE VERSION 0.1.0 LANGUAGES C CXX)

# ── Language standards ──────────────────────────────────────
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Platform integration modules ────────────────────────────
include(cmake/PDCurses.cmake)
include(cmake/imtui-patch.cmake)

# ── Targets ─────────────────────────────────────────────────
add_subdirectory(src/mpy)           # mpy: MicroPython embed + HAL + C++ wrapper
add_subdirectory(third_party/imtui)  # imtui-ncurses, imtui, imgui-for-imtui

# ── midle executable ────────────────────────────────────────
add_executable(midle
    src/main.cpp
    src/app.cpp
    src/io/file_io.cpp
    src/ui/tui.cpp
    src/ui/editor_panel.cpp
    src/ui/shell_panel.cpp
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/imtui/third-party/imgui/imgui/misc/cpp/imgui_stdlib.cpp"
)

target_link_libraries(midle PRIVATE
    mpy
    imtui-ncurses
)

target_include_directories(midle PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/imtui/third-party/imgui/imgui"
)

if(NOT MINGW)
    target_link_libraries(midle PRIVATE m)
endif()
if(MINGW)
    target_link_options(midle PRIVATE -static)
endif()

# ── Tests ───────────────────────────────────────────────────
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
