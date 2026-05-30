add_subdirectory(python)
add_subdirectory(javascript)

add_library(midle_language_registry STATIC register.cpp)

target_link_libraries(midle_language_registry PUBLIC
    midle_lang_python
    midle_lang_javascript
)

target_include_directories(midle_language_registry PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
)
