#ifndef MIDLE_COMPAT_PDCURSES_H
#define MIDLE_COMPAT_PDCURSES_H

#include <curses.h>

/* ImTUI's Windows backend defines ncurses-compatible key values after
   including pdcurses.h, so clear the PDCurses versions first. */
#undef KEY_CODE_YES
#undef KEY_BREAK
#undef KEY_DOWN
#undef KEY_UP
#undef KEY_LEFT
#undef KEY_RIGHT
#undef KEY_HOME
#undef KEY_BACKSPACE
#undef KEY_F0

#endif
