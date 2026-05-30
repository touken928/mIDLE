set(LUA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../third_party/lua")

set(LUA_CORE_SOURCES
    "${LUA_DIR}/lapi.c"
    "${LUA_DIR}/lauxlib.c"
    "${LUA_DIR}/lcode.c"
    "${LUA_DIR}/lctype.c"
    "${LUA_DIR}/ldebug.c"
    "${LUA_DIR}/ldo.c"
    "${LUA_DIR}/ldump.c"
    "${LUA_DIR}/lfunc.c"
    "${LUA_DIR}/lgc.c"
    "${LUA_DIR}/llex.c"
    "${LUA_DIR}/lmem.c"
    "${LUA_DIR}/lobject.c"
    "${LUA_DIR}/lopcodes.c"
    "${LUA_DIR}/lparser.c"
    "${LUA_DIR}/lstate.c"
    "${LUA_DIR}/lstring.c"
    "${LUA_DIR}/ltable.c"
    "${LUA_DIR}/ltm.c"
    "${LUA_DIR}/lundump.c"
    "${LUA_DIR}/lvm.c"
    "${LUA_DIR}/lzio.c"
)

set(LUA_LIB_SOURCES
    "${LUA_DIR}/linit.c"
    "${LUA_DIR}/lbaselib.c"
    "${LUA_DIR}/lcorolib.c"
    "${LUA_DIR}/ldblib.c"
    "${LUA_DIR}/liolib.c"
    "${LUA_DIR}/lmathlib.c"
    "${LUA_DIR}/loadlib.c"
    "${LUA_DIR}/loslib.c"
    "${LUA_DIR}/lstrlib.c"
    "${LUA_DIR}/ltablib.c"
    "${LUA_DIR}/lutf8lib.c"
)

add_library(midle_lua_core STATIC
    ${LUA_CORE_SOURCES}
    ${LUA_LIB_SOURCES}
)

target_include_directories(midle_lua_core PUBLIC "${LUA_DIR}")
target_compile_options(midle_lua_core PRIVATE -w)

find_package(Threads REQUIRED)

add_library(midle_lang_lua STATIC
    backend.cpp
    module.cpp
    highlight/tokenizer.cpp
)

target_link_libraries(midle_lang_lua PUBLIC midle_runtime_core midle_lua_core Threads::Threads)
if(NOT MINGW)
    target_link_libraries(midle_lang_lua PUBLIC m)
endif()

target_include_directories(midle_lang_lua PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../"
)
