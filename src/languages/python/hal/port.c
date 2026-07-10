#include <stddef.h>
#include <stdatomic.h>
#include <string.h>
#include "py/misc.h"
#include "py/mphal.h"
#include "py/compile.h"
#include "py/lexer.h"
#include "py/runtime.h"
#include "py/builtin.h"
#include "py/mperrno.h"
#include "shared/readline/readline.h"
#include "runtime/core/script_io.h"

static atomic_int port_cancel_requested;

void mp_hal_set_interrupt_char(int c) { (void)c; }

void port_clear_cancel_request(void) {
    atomic_store_explicit(&port_cancel_requested, 0, memory_order_release);
}

void port_request_cancel(void) {
    atomic_store_explicit(&port_cancel_requested, 1, memory_order_release);
}

void port_vm_hook_loop(void) {
    if (atomic_load_explicit(&port_cancel_requested, memory_order_acquire)) {
        mp_sched_keyboard_interrupt();
    }
}

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

int port_readline(struct _vstr_t *line, const char *prompt) {
    (void)prompt;
    const uint64_t generation = script_io_stdin_generation();
    for (;;) {
        unsigned char input_byte = 0;
        const int result = script_io_read_char_generation(generation, &input_byte);
        if (result == SCRIPT_IO_READ_CANCELLED) {
            return CHAR_CTRL_C;
        }
        if (result == SCRIPT_IO_READ_EOF) {
            return CHAR_CTRL_D;
        }
        int c = input_byte;
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

int port_exec_str(const char *src) {
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_lexer_t *lex = mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_, src, strlen(src), 0);
        qstr source_name = lex->source_name;
        mp_parse_tree_t parse_tree = mp_parse(lex, MP_PARSE_FILE_INPUT);
        mp_obj_t module_fun = mp_compile(&parse_tree, source_name, true);
        mp_call_function_0(module_fun);
        nlr_pop();
        return PORT_EXEC_SUCCEEDED;
    }

    if (mp_obj_is_subclass_fast(
            MP_OBJ_FROM_PTR(((mp_obj_base_t *)nlr.ret_val)->type),
            MP_OBJ_FROM_PTR(&mp_type_KeyboardInterrupt))) {
        if (atomic_load_explicit(&port_cancel_requested, memory_order_acquire)) {
            return PORT_EXEC_CANCELLED;
        }
        mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
        return PORT_EXEC_KEYBOARD_INTERRUPT;
    }
    mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
    return PORT_EXEC_FAILED;
}
