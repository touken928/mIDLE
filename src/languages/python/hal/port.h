#ifndef MICROPY_INCLUDED_MIDLE_PORT_H
#define MICROPY_INCLUDED_MIDLE_PORT_H

#define mp_hal_pin_obj_t void*

#ifdef __cplusplus
extern "C" {
#endif

void mp_hal_set_interrupt_char(int c);
struct _vstr_t;
int port_readline(struct _vstr_t *line, const char *prompt);

enum port_exec_result {
    PORT_EXEC_SUCCEEDED = 0,
    PORT_EXEC_CANCELLED = 1,
    PORT_EXEC_FAILED = 2,
    PORT_EXEC_KEYBOARD_INTERRUPT = 3,
};

void port_clear_cancel_request(void);
void port_request_cancel(void);
void port_vm_hook_loop(void);
int port_exec_str(const char *src);

#ifdef __cplusplus
}
#endif

#endif
