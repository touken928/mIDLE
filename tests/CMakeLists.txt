# Disable Google Test's own tests and install
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../third_party/googletest" "${CMAKE_CURRENT_BINARY_DIR}/googletest")

# ── Test: mpy library ───────────────────────────────────────
add_executable(test_mpy
    test_mpy.cpp
)

target_link_libraries(test_mpy PRIVATE mpy gtest_main)

target_include_directories(test_mpy PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../src"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/mpy"
)

add_test(NAME test_mpy COMMAND test_mpy)
