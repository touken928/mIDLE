set(QJS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../third_party/quickjs")

add_library(midle_quickjs_core STATIC
    "${QJS_DIR}/quickjs.c"
    "${QJS_DIR}/dtoa.c"
    "${QJS_DIR}/libregexp.c"
    "${QJS_DIR}/libunicode.c"
    "${QJS_DIR}/cutils.c"
)

file(READ "${QJS_DIR}/VERSION" QJS_VERSION)
string(STRIP "${QJS_VERSION}" QJS_VERSION)

target_compile_definitions(midle_quickjs_core PRIVATE CONFIG_BIGNUM=0 CONFIG_VERSION=\"${QJS_VERSION}\")
if(MINGW)
    target_compile_definitions(midle_quickjs_core PRIVATE _WIN32 __USE_MINGW_ANSI_STDIO)
else()
    target_compile_definitions(midle_quickjs_core PRIVATE _GNU_SOURCE)
endif()

target_include_directories(midle_quickjs_core PRIVATE "${QJS_DIR}")
target_compile_options(midle_quickjs_core PRIVATE -w)

find_package(Threads REQUIRED)

add_library(midle_lang_javascript STATIC
    backend.cpp
    module.cpp
    highlight/tokenizer.cpp
)

target_link_libraries(midle_lang_javascript PUBLIC midle_runtime_core midle_quickjs_core Threads::Threads)

target_include_directories(midle_lang_javascript PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../"
)
