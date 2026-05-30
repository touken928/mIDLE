set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../third_party/googletest" "${CMAKE_CURRENT_BINARY_DIR}/googletest")

add_executable(test_python_backend test_python_backend.cpp)
target_link_libraries(test_python_backend PRIVATE midle_lang_python gtest_main)
target_include_directories(test_python_backend PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../src")
add_test(NAME test_python_backend COMMAND test_python_backend)

add_executable(test_javascript_backend test_javascript_backend.cpp)
target_link_libraries(test_javascript_backend PRIVATE midle_lang_javascript gtest_main)
target_include_directories(test_javascript_backend PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../src")
add_test(NAME test_javascript_backend COMMAND test_javascript_backend)

add_executable(test_runtime test_runtime.cpp)
target_link_libraries(test_runtime PRIVATE midle_runtime gtest_main)
target_include_directories(test_runtime PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../src")
add_test(NAME test_runtime COMMAND test_runtime)

add_executable(test_highlight test_highlight.cpp)
target_link_libraries(test_highlight PRIVATE midle_lang_python midle_lang_javascript gtest_main)
target_include_directories(test_highlight PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../src")
add_test(NAME test_highlight COMMAND test_highlight)
