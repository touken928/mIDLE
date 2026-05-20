function(midle_add_imtui_ncurses)
    if(NOT IMTUI_NCURSES_PATCHED_CPP)
        message(FATAL_ERROR "IMTUI_NCURSES_PATCHED_CPP unset; include cmake/imtui-patch.cmake first")
    endif()

    if(MINGW)
        if(NOT TARGET pdcurses)
            message(FATAL_ERROR "pdcurses target missing")
        endif()
        set(_curses_libs pdcurses)
        set(_curses_include "${CURSES_INCLUDE_DIR}")
    else()
        find_package(Curses REQUIRED)
        set(_curses_libs ${CURSES_LIBRARIES})
        set(_curses_include ${CURSES_INCLUDE_DIR})
    endif()

    add_library(imtui-ncurses STATIC "${IMTUI_NCURSES_PATCHED_CPP}")
    target_include_directories(imtui-ncurses
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/third_party/imtui/include"
        PRIVATE ${_curses_include}
    )
    target_link_libraries(imtui-ncurses PUBLIC imtui ${_curses_libs})
endfunction()
