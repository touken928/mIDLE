# PDCurses target — Windows console backend for ImTUI.
# Only needed on MinGW; on other platforms ncurses is provided by the system.

if(MINGW)
    file(GLOB PDCURSES_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/pdcurses/pdcurses/*.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/pdcurses/wincon/*.c"
    )

    add_library(pdcurses STATIC ${PDCURSES_SOURCES})
    target_include_directories(pdcurses PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/src/compat"
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/pdcurses"
    )
    target_compile_definitions(pdcurses PUBLIC PDC_FORCE_UTF8)

    set(CURSES_INCLUDE_DIR
        "${CMAKE_CURRENT_SOURCE_DIR}/src/compat;${CMAKE_CURRENT_SOURCE_DIR}/third_party/pdcurses"
        CACHE STRING "PDCurses include directories" FORCE
    )
endif()
