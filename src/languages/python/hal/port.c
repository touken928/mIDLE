#include <stddef.h>
#include "py/misc.h"
#include "py/mphal.h"
#include "py/runtime.h"
#include "py/builtin.h"
#include "py/mperrno.h"
#include "shared/readline/readline.h"
#include "runtime/core/script_io.h"

void mp_hal_set_interrupt_char(int c) { (void)c; }

mp_obj_t mp_builtin_open(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    (void)n_args;
    (void)args;
    (void)kw_args;
    mp_raise_OSError(MP_ENOENT);
}
MP_DEFINE_CONST_FUN_OBJ_KW(mp_builtin_open_obj, 0, mp_builtin_open);

void mp_hal_stdout_tx_strn_cooked(const char *str, size_t len) {
    script_io_write(str, len);
}

int mp_hal_stdin_rx_chr(void) {
    return script_io_read_char();
}

int port_readline(vstr_t *line, const char *prompt) {
    (void)prompt;
    for (;;) {
        int c = mp_hal_stdin_rx_chr();
        if (c < 0) {
            return CHAR_CTRL_D;
        }
        if (c == CHAR_CTRL_C) {
            return CHAR_CTRL_C;
        }
        if (c == '\r') {
            continue;
        }
        if (c == '\n') {
            return 0;
        }
        vstr_add_byte(line, (byte)c);
    }
}
