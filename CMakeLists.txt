cmake_minimum_required(VERSION 3.16)
project(mIDLE VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ═════════════════════════════════════════════════════════════
# 1.  MicroPython embedding target  (micropy + mp_core)
# ═════════════════════════════════════════════════════════════
add_subdirectory(src/mpy)

# ═════════════════════════════════════════════════════════════
# 2.  ImTUI
# ═════════════════════════════════════════════════════════════
add_subdirectory(third_party/imtui)

# ═════════════════════════════════════════════════════════════
# 3.  mIDLE executable
# ═════════════════════════════════════════════════════════════
add_executable(midle
    src/main.cpp
    src/app.cpp
    src/ui/editor_panel.cpp
    src/ui/shell_panel.cpp
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/imtui/third-party/imgui/imgui/misc/cpp/imgui_stdlib.cpp"
)
target_link_libraries(midle PRIVATE
    mpy
    imtui-ncurses
    m
)
target_include_directories(midle PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/imtui/third-party/imgui/imgui"
)
