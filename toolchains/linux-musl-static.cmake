# Target: Linux x64 fully-static musl binary
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_EXE_LINKER_FLAGS "-static" CACHE STRING "Static link flags" FORCE)
