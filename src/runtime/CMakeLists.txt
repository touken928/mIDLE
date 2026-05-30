add_library(midle_runtime_core STATIC
    core/script_io.c
    core/async_runner.cpp
    core/registry.cpp
)

find_package(Threads REQUIRED)
target_link_libraries(midle_runtime_core PUBLIC Threads::Threads)

target_include_directories(midle_runtime_core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
)
