cmake_minimum_required(VERSION 3.16)
project(mIDLE VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(cmake/PDCurses.cmake)
include(cmake/imtui-patch.cmake)

add_subdirectory(src/runtime)
add_subdirectory(src/languages)

add_library(midle_runtime INTERFACE)
target_link_libraries(midle_runtime INTERFACE
    midle_runtime_core
    midle_lang_python
    midle_lang_javascript
)
target_include_directories(midle_runtime INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

set(IMTUI_SUPPORT_NCURSES OFF CACHE BOOL "midle supplies patched imtui-ncurses" FORCE)
add_subdirectory(third_party/imtui)
include(cmake/imtui-ncurses.cmake)
midle_add_imtui_ncurses()

add_executable(midle
    src/main.cpp
    src/app.cpp
    src/io/file_io.cpp
    src/ui/tui.cpp
    src/highlight/ansi_colors.cpp
    src/highlight/screen.cpp
    src/ui/editor_panel.cpp
    src/ui/shell_panel.cpp
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/imtui/third-party/imgui/imgui/misc/cpp/imgui_stdlib.cpp"
)

target_link_libraries(midle PRIVATE
    midle_runtime
    imtui-ncurses
)

target_include_directories(midle PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/imtui/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/imtui/third-party/imgui/imgui"
)

if(NOT MINGW)
    target_link_libraries(midle PRIVATE m)
endif()
if(MINGW)
    target_link_options(midle PRIVATE -static)
endif()

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
